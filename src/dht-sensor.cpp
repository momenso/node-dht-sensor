#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "bcm2835/bcm2835.h"
#include <unistd.h>

#define BCM2708_PERI_BASE   0x20000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000)
#define MAXTIMINGS          100
#define DHT11               11
#define DHT22               22
#define AM2302              22

int initialized = 0;
unsigned long long last_read[32] = {};
float last_temperature[32] = {};
float last_humidity[32] = {};

unsigned long long getTime()
{
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  unsigned long long time = (unsigned long long)(tv.tv_sec) * 1000 +
                            (unsigned long long)(tv.tv_usec) / 1000;
  return time;
}

long readDHT(int type, int pin, float &temperature, float &humidity)
{
  int bitCount = 0;
  int timeout;
  int bits[MAXTIMINGS];
  int data[MAXTIMINGS / 8] = {};

  #ifdef VERBOSE
  #ifdef DBG_CONSOLE
  FILE *pTrace = stdout;
  #else
  FILE *pTrace = fopen("dht-sensor.log", "a");
  if (pTrace == nullptr)
  {
    puts("WARNING: unable to initialize trace file, it will be redirected to stdout.");
    pTrace = stdout;
  }
  #endif
  #endif

  #ifdef VERBOSE
  fprintf(pTrace, "start sensor read (type=%d, pin=%d).\n", type, pin);
  #endif

  // throttle sensor reading - if last read was less than 2s then return same
  unsigned long long now = getTime();
  if ((last_read[pin] > 0) && (now - last_read[pin] < 3000))
  {
    #ifdef VERBOSE
    fprintf(pTrace, "*** too early to read again pin %d: %llu\n", pin, now - last_read[pin]);
    #endif
    temperature = last_temperature[pin];
    humidity = last_humidity[pin];
    return 0;
  }

  // request sensor data
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_write(pin, HIGH);
  usleep(10000);
  bcm2835_gpio_write(pin, LOW);
  usleep(type == 11 ? 18000 : 800);
  bcm2835_gpio_write(pin, HIGH);
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);

  // wait for sensor response
  for (timeout = 0; timeout < 1000000 && bcm2835_gpio_lev(pin) == LOW; ++timeout);
  if (timeout >= 100000)
  {
    #ifdef VERBOSE
    fprintf(pTrace, "*** timeout #1\n");
    #ifdef DBG_CONSOLE
    fflush(pTrace);
    #else
    fclose(pTrace);
    #endif
    #endif
    return -3;
  }

  for (timeout = 0; timeout < 1000000 && bcm2835_gpio_lev(pin) == HIGH; ++timeout);
  if (timeout >= 100000)
  {
    #ifdef VERBOSE
    fprintf(pTrace, "*** timeout #2\n");
    #ifdef DBG_CONSOLE
    fflush(pTrace);
    #else
    fclose(pTrace);
    #endif
    #endif
    return -3;
  }

  // read data
  for (bitCount = 0; bitCount < MAXTIMINGS; ++bitCount)
  {
    for (timeout = 0; bcm2835_gpio_lev(pin) == LOW && timeout < 50000; ++timeout);
    for (timeout = 0; bcm2835_gpio_lev(pin) == HIGH && timeout < 50000; ++timeout);
    bits[bitCount] = timeout;
    if (timeout >= 50000) break;
  }

  // data decoding, get widest pulse
  int peak = bits[1];
  #ifdef VERBOSE
  fprintf(pTrace, "init peak: %d\n", bits[1]);
  #endif
  for (int i = 2; i < bitCount; ++i)
  {
    if (peak < bits[i])
    {
      peak = bits[i];
      #ifdef VERBOSE
      fprintf(pTrace, "update peak: %d (%d)\n", i, bits[i]);
      #endif
    }
  }

  // convert pulses to bits
  #ifdef VERBOSE
  fprintf(pTrace, "bitCount=%d, peak=%d:\n", bitCount, peak);
  #endif
  int k = 0;
  for (int i = 1; i < bitCount; ++i)
  {
    data[k] <<= 1;
    if ((2 * bits[i] - peak) > 0)
    {
      data[k] |= 1;
      #ifdef VERBOSE
      fprintf(pTrace, "1 (%03d) ", bits[i]);
    }
    else
    {
      fprintf(pTrace, "0 (%03d) ", bits[i]);
      #endif
    }
    if (i % 8 == 0)
    {
      k++;
      #ifdef VERBOSE
      fprintf(pTrace, "\n");
      #endif
    }
  }

  // crc checking
  #ifdef VERBOSE
  int crc = ((data[0] + data[1] + data[2] + data[3]) & 0xff);
  fprintf(pTrace, "\n=> %x %x %x %x (%x/%x) : %s\n",
          data[0], data[1], data[2], data[3], data[4],
          crc, (data[4] == crc) ? "OK" : "ERR");
  #endif

  // checks if received data is valid:
  // - minimum of 5 bytes (40 bits)
  // - has to be whole bytes
  // - checksum must match
  if ((bitCount > 40) && ((bitCount - 1) % 8 == 0) &&
      (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xff)))
  {
    #ifdef VERBOSE
    fprintf(pTrace, "sensor type = %d, ", type);
    #endif

    if (type == DHT11)
    {
      #ifdef VERBOSE
      printf("temperature = %d, humidity = %d\n", data[2], data[0]);
      #endif
      temperature = data[2];
      humidity = data[0];
    }
    else if (type == DHT22)
    {
      float f, h;
      h = data[0] * 256 + data[1];
      h /= 10.0;

      f = (data[2] & 0x7F) * 256 + data[3];
      f /= 10.0;
      if (data[2] & 0x80) f *= -1;

      #ifdef VERBOSE
      fprintf(pTrace, "temperature = %.1f, humidity = %.1f\n", f, h);
      #endif
      temperature = f;
      humidity = h;
    }
    else
    {
      #ifdef VERBOSE
      fprintf(pTrace, "*** unknown sensor data type\n");
      #ifdef DBG_CONSOLE
      fflush(pTrace);
      #else
      fclose(pTrace);
      #endif
      #endif
      return -2;
    }
  }
  else
  {
    #ifdef VERBOSE
    fprintf(pTrace, "*** unexpected data: bits=%d: %d != %d + %d + %d + %d\n",
    bitCount, data[4], data[0], data[1], data[2], data[3]);
    #ifdef DBG_CONSOLE
    fflush(pTrace);
    #else
    fclose(pTrace);
    #endif
    #endif
    return -1;
  }

  #ifdef VERBOSE
  fprintf(pTrace, "*** obtained readout successfully.\n");
  #ifdef DBG_CONSOLE
  fflush(pTrace);
  #else
  fclose(pTrace);
  #endif
  #endif

  // update last readout
  last_read[pin] = now;
  last_temperature[pin] = temperature;
  last_humidity[pin] = humidity;

  return 0;
}

int initialize()
{
  if (!bcm2835_init())
  {
    #ifdef VERBOSE
    puts("BCM2835 initialization failed.");
    #endif
    return 1;
  }
  else
  {
    #ifdef VERBOSE
    puts("BCM2835 initialized.");
      #endif
      initialized = 1;
      memset(last_read, 0, sizeof(unsigned long long)*32);
      memset(last_temperature, 0, sizeof(float)*32);
      memset(last_humidity, 0, sizeof(float)*32);
      return 0;
  }
}
