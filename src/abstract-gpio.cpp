#include "abstract-gpio.h"

#include <algorithm>
#include <fstream>
#include <regex>
#include <stdio.h>
#include <string>

#ifdef USE_LIBGPIOD
#include <cstring>
#include <gpiod.h>
#else
#include "bcm2835/bcm2835.h"
#endif

#ifdef USE_LIBGPIOD
static gpiod_chip *theChip = NULL;
static GpioDirection lastDirection[MAX_GPIO_PIN_NUMBER + 1];

#ifdef GPIOD_V2
static gpiod_line_request *requests[MAX_GPIO_PIN_NUMBER + 1];
#else
static gpiod_line *lines[MAX_GPIO_PIN_NUMBER + 1];
#endif

static void gpiodCleanUp()
{
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
}

static gpiod_chip* findGpioChip()
{
  gpiod_chip* chip = NULL;

  // 1. Safely enumerate up to 10 chips looking for the RP1 southbridge (Pi 5)
  for (int i = 0; i < 10; i++)
  {
    char path[32];
    snprintf(path, sizeof(path), "/dev/gpiochip%d", i);

    #ifdef GPIOD_V2
    chip = gpiod_chip_open(path);
    if (chip != NULL)
    {
      gpiod_chip_info* info = gpiod_chip_get_info(chip);
      if (info != NULL)
      {
        const char* label = gpiod_chip_info_get_label(info);
        bool is_rp1 = (label != NULL && strstr(label, "pinctrl-rp1") != NULL);
        gpiod_chip_info_free(info);

        if (is_rp1) return chip;
      }
      gpiod_chip_close(chip);
    }
    #else
    chip = gpiod_chip_open(path);
    if (chip != NULL)
    {
      const char* label = gpiod_chip_label(chip);
      if (label != NULL && strstr(label, "pinctrl-rp1") != NULL)
      {
        return chip;
      }
      gpiod_chip_close(chip);
    }
    #endif
  }

  // 2. Fallback for older Pi's using libgpiod or default setups
  #ifdef GPIOD_V2
  chip = gpiod_chip_open("/dev/gpiochip4");
  if (chip == NULL) chip = gpiod_chip_open("/dev/gpiochip0");
  #else
  chip = gpiod_chip_open_by_name("gpiochip4");
  if (chip == NULL) chip = gpiod_chip_open_by_name("gpiochip0");
  #endif

  return chip;
}

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

    if (requests[pin] != NULL)
    {
      lastDirection[pin] = GPIO_OUT;
    }
  }

  return requests[pin];
}
#else
static gpiod_line* getLine(int pin, GpioDirection direction)
{
  if (lines[pin] != NULL && lastDirection[pin] != direction)
  {
    gpiod_line_release(lines[pin]);
    lines[pin] = NULL;
  }

  if (lines[pin] == NULL)
  {
    lines[pin] = gpiod_chip_get_line(theChip, pin);
    if (lines[pin] == NULL) return NULL;

    if (direction == GPIO_IN) { gpiod_line_request_input(lines[pin], "node-dht-sensor"); }
    else { gpiod_line_request_output(lines[pin], "node-dht-sensor", 0); }
    lastDirection[pin] = direction;
  }
  return lines[pin];
}
#endif // GPIOD_V2

// --- bcm2835 State ---
#else
static GpioDirection lastDirection[MAX_GPIO_PIN_NUMBER + 1];
#endif // USE_LIBGPIOD

// --- Public API ---

bool gpioInitialize()
{
#ifdef USE_LIBGPIOD
  theChip = findGpioChip();

  if (theChip == NULL)
  {
    #ifdef VERBOSE
    puts("libgpiod initialization failed: Could not find RP1 or fallback chips.");
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
    std::atexit(gpiodCleanUp);
    return true;
  }
#else
  if (!bcm2835_init())
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
    std::fill(lastDirection, lastDirection + MAX_GPIO_PIN_NUMBER + 1, GPIO_UNSET);
    return true;
  }
#endif
}

void gpioWrite(int pin, GpioPinState state)
{
#ifdef USE_LIBGPIOD
  #ifdef GPIOD_V2
  gpiod_line_request *req = getLineRequest(pin);
  if (req != NULL) 
  {
    gpiod_line_request_set_value(req, pin, state == GPIO_LOW ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE);
    lastDirection[pin] = GPIO_OUT;
  }
  #else
  gpiod_line *line = getLine(pin, GPIO_OUT);
  if (line != NULL) 
  {
    gpiod_line_set_value(line, state == GPIO_LOW ? 0 : 1);
  }
  #endif
#else
  if (lastDirection[pin] != GPIO_OUT)
  {
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
    lastDirection[pin] = GPIO_OUT;
  }
  bcm2835_gpio_write(pin, state == GPIO_LOW ? LOW : HIGH);
#endif
}

GpioPinState gpioRead(int pin)
{
#ifdef USE_LIBGPIOD
  #ifdef GPIOD_V2
  gpiod_line_request *req = getLineRequest(pin);
  if (req == NULL) return GPIO_HIGH;

  if (lastDirection[pin] == GPIO_OUT)
  {
    gpiod_line_request_set_value(req, pin, GPIOD_LINE_VALUE_ACTIVE);
    lastDirection[pin] = GPIO_IN;
  }

  int val = gpiod_line_request_get_value(req, pin);

  if (val == -1)
  {
    return GPIO_HIGH;
  }
  return val == GPIOD_LINE_VALUE_INACTIVE ? GPIO_LOW : GPIO_HIGH;

  #else
  gpiod_line *line = getLine(pin, GPIO_IN);
  if (line == NULL) return GPIO_HIGH;

  int val = gpiod_line_get_value(line);
  if (val == -1)
  {
    return GPIO_HIGH;
  }
  return val == 0 ? GPIO_LOW : GPIO_HIGH;
  #endif
#else
  if (lastDirection[pin] != GPIO_IN)
  {
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
    lastDirection[pin] = GPIO_IN;
  }
  return bcm2835_gpio_lev(pin) == LOW ? GPIO_LOW : GPIO_HIGH;
#endif
}
