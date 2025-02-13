#ifndef ABSTRACT_GPIO
#define ABSTRACT_GPIO

enum GpioDirection { GPIO_IN, GPIO_OUT };
enum GpioPinState { GPIO_LOW, GPIO_HIGH };

int gpioInitialize();
void gpioSetDirection(int pin, GpioDirection direction);
void gpioWrite(int pin, GpioPinState state);
GpioPinState gpioRead(int pin);

#endif
