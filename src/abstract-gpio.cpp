#include "abstract-gpio.h"

#include <algorithm>
#include "bcm2835/bcm2835.h"
#include <fstream>
#include <regex>
#include <stdio.h>
#include <string>

#ifdef USE_LIBGPIOD
#include <gpiod.h>
#endif

static bool useGpiod = false;
static GpioDirection lastDirection[MAX_GPIO_PIN_NUMBER + 1];
#ifdef USE_LIBGPIOD
static gpiod_chip *theChip = NULL;
static gpiod_line *lines[MAX_GPIO_PIN_NUMBER + 1];
#endif

static void gpiodCleanUp()
{
  #ifdef USE_LIBGPIOD
  for (int i = 1; i <= MAX_GPIO_PIN_NUMBER; ++i)
  {
    if (lines[i] != NULL)
    {
      gpiod_line_release(lines[i]);
    }
  }

  gpiod_chip_close(theChip);
  #endif
}

bool gpioInitialize()
{
  bool isPi5 = false;
  std::ifstream file("/proc/cpuinfo");
  std::string line;

  if (file.is_open())
  {
    std::regex pattern(R"(Model\s+:\s+Raspberry Pi (\d+))");
    std::smatch match;

    while (std::getline(file, line))
    {
      if (std::regex_search(line, match, pattern) && std::stoi(match[1]) > 4)
      {
        #ifdef USE_LIBGPIOD
        isPi5 = true;
        #endif
        break;
      }
    }

    file.close();
  }

  if (isPi5)
  {
    #ifdef USE_LIBGPIOD
    theChip = gpiod_chip_open_by_name("gpiochip0");

    if (theChip == NULL)
    {
      #ifdef VERBOSE
      puts("libgpiod initialization failed.");
      #endif
      return false;
    }
    else
    {
      #ifdef VERBOSE
      puts("libgpiod initialized.");
      #endif
      std::fill(lines, lines + MAX_GPIO_PIN_NUMBER + 1, (gpiod_line*) NULL);
      std::fill(lastDirection, lastDirection + MAX_GPIO_PIN_NUMBER + 1, GPIO_UNSET);
      useGpiod = true;
      std::atexit(gpiodCleanUp);
      return true;
    }
    #endif
  }
  else if (!bcm2835_init())
  {
    #ifdef VERBOSE
    puts("BCM2835 initialization failed.");
    #endif
    return false;
  }
  else
  {
    #ifdef VERBOSE
    puts("BCM2835 initialized.");
    #endif
    return true;
  }
}

#ifdef USE_LIBGPIOD
static gpiod_line* getLine(int pin, GpioDirection direction)
{
  if (lines[pin] == NULL || lastDirection[pin] != direction)
  {
    if (lines[pin] != NULL)
    {
      gpiod_line_release(lines[pin]);
      lines[pin] = NULL;
    }

    lines[pin] = gpiod_chip_get_line(theChip, pin);
  }

  gpiod_line* line = lines[pin];

  if (lastDirection[pin] != direction)
  {
    if (direction == GPIO_IN)
    {
      gpiod_line_request_input(line, "Consumer");
    }
    else
    {
      gpiod_line_request_output(line, "Consumer", 0);
    }

    lastDirection[pin] = direction;
  }

  return line;
}
#endif

void gpioWrite(int pin, GpioPinState state)
{
  if (useGpiod)
  {
    #ifdef USE_LIBGPIOD
    gpiod_line_set_value(getLine(pin, GPIO_OUT), state == GPIO_LOW ? 0 : 1);
    #endif
  }
  else
  {
    if (lastDirection[pin] != GPIO_OUT)
    {
      bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
      lastDirection[pin] = GPIO_OUT;
    }

    bcm2835_gpio_write(pin, state == GPIO_LOW ? LOW : HIGH);
  }
}

GpioPinState gpioRead(int pin)
{
  if (useGpiod)
  {
    #ifdef USE_LIBGPIOD
    return gpiod_line_get_value(getLine(pin, GPIO_IN)) == 0 ? GPIO_LOW : GPIO_HIGH;
    #endif
  }
  else
  {
    if (lastDirection[pin] != GPIO_IN)
    {
      bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
      lastDirection[pin] = GPIO_IN;
    }

    return bcm2835_gpio_lev(pin) == LOW ? GPIO_LOW : GPIO_HIGH;
  }
}
