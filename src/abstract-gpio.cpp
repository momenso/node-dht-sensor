#include "abstract-gpio.h"

#include <algorithm>
#include <stdio.h>
#include "bcm2835/bcm2835.h"
#include <gpiod.h>

#define MAX_LINES 100

static bool useGpiod = false;
static gpiod_chip *theChip = NULL;
static gpiod_line *lines[MAX_LINES + 1];
static GpioDirection lastDirection[MAX_LINES + 1];

int gpioInitialize()
{
  if (!bcm2835_init())
  {
    #ifdef VERBOSE
    puts("BCM2835 initialization failed.");
    #endif

    theChip = gpiod_chip_open_by_name("gpiochip0");

    if (theChip == NULL)
    {
      #ifdef VERBOSE
      puts("libgpiod initialization failed.");
      #endif
      return 1;
    }
    else
    {
      #ifdef VERBOSE
      puts("libgpiod initialized.");
      #endif
      std::fill(lines, lines + MAX_LINES + 1, (gpiod_line*) NULL);
      std::fill(lastDirection, lastDirection + MAX_LINES + 1, GPIO_UNSET);
      useGpiod = true;
      return 0;
    }
  }
  else
  {
    #ifdef VERBOSE
    puts("BCM2835 initialized.");
    #endif
    return 0;
  }
}

static gpiod_line* getLine(int pin, GpioDirection direction)
{
  if (lines[pin] == NULL)
  {
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

void gpioWrite(int pin, GpioPinState state)
{
  if (useGpiod)
  {
    gpiod_line_set_value(getLine(pin, GPIO_OUT), state == GPIO_LOW ? 0 : 1);
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
    return gpiod_line_get_value(getLine(pin, GPIO_IN)) == 0 ? GPIO_LOW : GPIO_HIGH;
  }
  else {
    if (lastDirection[pin] != GPIO_IN)
    {
      bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
      lastDirection[pin] = GPIO_IN;
    }

    return bcm2835_gpio_lev(pin) == LOW ? GPIO_LOW : GPIO_HIGH;
  }
}
