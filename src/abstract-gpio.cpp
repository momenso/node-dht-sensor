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

#ifdef GPIOD_V2
static gpiod_line_request *requests[MAX_GPIO_PIN_NUMBER + 1];
#else
static gpiod_line *lines[MAX_GPIO_PIN_NUMBER + 1];
#endif
#endif

static void gpiodCleanUp()
{
  #ifdef USE_LIBGPIOD
  for (int i = 1; i <= MAX_GPIO_PIN_NUMBER; ++i)
  {
    #ifdef GPIOD_V2
    if (requests[i] != NULL)
    {
      gpiod_line_request_release(requests[i]);
    }
    #else
    if (lines[i] != NULL)
    {
      gpiod_line_release(lines[i]);
    }
    #endif
  }

  if (theChip != NULL)
  {
    gpiod_chip_close(theChip);
  }
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

    #ifdef GPIOD_V2
    // libgpiod v2 requires paths instead of names.
    // On the Pi 5, the main header is typically on gpiochip4 (RP1 southbridge)
    theChip = gpiod_chip_open("/dev/gpiochip4");
    if (theChip == NULL) {
      theChip = gpiod_chip_open("/dev/gpiochip0");
    }
    #else
    theChip = gpiod_chip_open_by_name("gpiochip0");
    #endif

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

      #ifdef GPIOD_V2
      std::fill(requests, requests + MAX_GPIO_PIN_NUMBER + 1, (gpiod_line_request*) NULL);
      #else
      std::fill(lines, lines + MAX_GPIO_PIN_NUMBER + 1, (gpiod_line*) NULL);
      #endif

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

#ifdef GPIOD_V2
static gpiod_line_request* getLineRequest(int pin)
{
  if (requests[pin] == NULL)
  {
    gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_drive(settings, GPIOD_LINE_DRIVE_OPEN_DRAIN);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);

    gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = pin;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "node-dht-sensor");

    requests[pin] = gpiod_chip_request_lines(theChip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    lastDirection[pin] = GPIO_IN;
  }

  return requests[pin];
}
#else
static gpiod_line* getLine(int pin, GpioDirection direction)
{
  if (lines[pin] == NULL)
  {
    lines[pin] = gpiod_chip_get_line(theChip, pin);
    if (direction == GPIO_IN)
    {
      gpiod_line_request_input(lines[pin], "node-dht-sensor");
    }
    else
    {
      gpiod_line_request_output(lines[pin], "node-dht-sensor", 0);
    }
    lastDirection[pin] = direction;
  }
  else if (lastDirection[pin] != direction)
  {
    if (direction == GPIO_IN)
    {
      gpiod_line_set_direction_input(lines[pin]);
    }
    else
    {
      gpiod_line_set_direction_output(lines[pin], 0);
    }
    lastDirection[pin] = direction;
  }
  return lines[pin];
}
#endif

#endif // USE_LIBGPIOD

void gpioWrite(int pin, GpioPinState state)
{
  if (useGpiod)
  {
    #ifdef USE_LIBGPIOD
    #ifdef GPIOD_V2
    gpiod_line_request_set_value(
      getLineRequest(pin),
      pin,
      state == GPIO_LOW ?
        GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE
    );
    lastDirection[pin] = GPIO_OUT;
    #else
    gpiod_line_set_value(
      getLine(pin, GPIO_OUT),
      state == GPIO_LOW ? 0 : 1
    );
    #endif
    #endif // USE_LIBGPIOD
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
    #ifdef GPIOD_V2
    if (lastDirection[pin] == GPIO_OUT)
    {
      gpiod_line_request_set_value(getLineRequest(pin), pin, GPIOD_LINE_VALUE_ACTIVE);
      lastDirection[pin] = GPIO_IN;
    }
    int val = gpiod_line_request_get_value(getLineRequest(pin), pin);
    return val == GPIOD_LINE_VALUE_INACTIVE ? GPIO_LOW : GPIO_HIGH;
    #else
    return gpiod_line_get_value(getLine(pin, GPIO_IN)) == 0 ? GPIO_LOW : GPIO_HIGH;
    #endif
    #endif // USE_LIBGPIOD
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
