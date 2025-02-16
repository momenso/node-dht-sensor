#ifndef ABSTRACT_GPIO
#define ABSTRACT_GPIO

#define MAX_GPIO_PIN_NUMBER 63

enum GpioDirection { GPIO_UNSET = -1, GPIO_IN, GPIO_OUT };
enum GpioPinState { GPIO_READ_FAILED = -1, GPIO_LOW, GPIO_HIGH };

bool gpioInitialize();
void gpioWrite(int pin, GpioPinState state);
GpioPinState gpioRead(int pin);

#endif
