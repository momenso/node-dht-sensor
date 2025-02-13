#include "abstract-gpio.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "bcm2835/bcm2835.h"
#include <unistd.h>

int gpioInitialize()
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
    return 0;
  }
}

void gpioSetDirection(int pin, GpioDirection direction)
{
  if (direction == GPIO_IN)
  {
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
  }
  else
  {
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
  }
}

void gpioWrite(int pin, GpioPinState state)
{
  if (state == GPIO_LOW)
  {
    bcm2835_gpio_write(pin, LOW);
  }
  else
  {
    bcm2835_gpio_write(pin, HIGH);
  }
}

GpioPinState gpioRead(int pin)
{
  return bcm2835_gpio_lev(pin) == LOW ? GPIO_LOW : GPIO_HIGH;
}
