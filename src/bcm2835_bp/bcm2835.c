// from https://github.com/LeMaker/bcm2835_BP/tree/master/src

// bcm2835.c
// C and C++ support for Broadcom BCM 2835 as used in Raspberry Pi
// http://elinux.org/RPi_Low-level_peripherals
// http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
//
// Author: Mike McCauley
// Copyright (C) 2011-2013 Mike McCauley
// $Id: bcm2835.c,v 1.16 2014/08/21 01:26:42 mikem Exp mikem $


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <linux/spi/spidev.h> /*Add for BananaPro by LeMaker Team*/
#include <sys/ioctl.h>  /*Add for BananaPro by LeMaker Team*/
#include <sys/time.h>   /*Add for BananaPro by LeMaker Team*/

#include "bcm2835.h"

// This define enables a little test program (by default a blinking output on pin RPI_GPIO_PIN_11)
// You can do some safe, non-destructive testing on any platform with:
// gcc bcm2835.c -D BCM2835_TEST
// ./a.out
//#define BCM2835_TEST

// Uncommenting this define compiles alternative I2C code for the version 1 RPi
// The P1 header I2C pins are connected to SDA0 and SCL0 on V1.
// By default I2C code is generated for the V2 RPi which has SDA1 and SCL1 connected.
// #define I2C_V1

/*Add for BananaPro by LeMaker Team*/
#define GPIONUM  28
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 500000;
static uint16_t delay;
static uint8_t  channel = 0; /*default CE0*/

static int32_t BpiPin[GPIONUM] =
{
	 257,256,   //BCM GPIO0,1
	 53,52,      //BCM GPIO2,3
	 226,35,     //BCM GPIO4,5
	 277,270,   //BCM GPIO6,7
	 266,269,   //BCM GPIO8,9
	 268,267,   //BCM GPIO10,11
	 276,45,     //BCM GPIO12,13
	 228,229,   //BCM GPIO14,15
	 38,275,     //BCM GPIO16,17
	 259,39,     //BCM GPIO18,19
	 44, 40,     //BCM GPIO20,21
	 273,244,   //BCM GPIO22,23
	 245,272,   //BCM GPIO24,25
	 37, 274,   //BCM GPIO26,27
};
static uint8_t GpioDetectType[32] = {0x0};
static uint32_t ReadRisingCnt = 0;
static uint32_t ReadFallCnt = 0;
static uint8_t RisingModeValue = 1;
static uint8_t FallModeValue = 0;

union i2c_smbus_data
{
  uint8_t  byte ;
  uint16_t word ;
  uint8_t  block [I2C_SMBUS_BLOCK_MAX + 2] ;	// block [0] is used for length + one more for PEC
} ;

struct i2c_smbus_ioctl_data
{
  char read_write ;
  uint8_t command ;
  int size ;
  union i2c_smbus_data *data ;
} ;
/*end 2015.01.05*/

/*Add for BananaPro by LeMaker Team*/
volatile uint32_t *sunxi_gpio  =  (volatile uint32_t *)MAP_FAILED;
volatile uint32_t *sunxi_pwm = (volatile uint32_t *)MAP_FAILED;
volatile uint32_t *sunxi_spi0  = (volatile uint32_t *)MAP_FAILED;
volatile uint32_t *sunxi_iic2  = (volatile uint32_t *)MAP_FAILED;

/*end 2015.01.05 */

// This variable allows us to test on hardware other than RPi.
// It prevents access to the kernel memory, and does not do any peripheral access
// Instead it prints out what it _would_ do if debug were 0
static uint8_t debug = 0;

// I2C The time needed to transmit one byte. In microseconds.
static int i2c_byte_wait_us = 0;

/*Add for BananaPro by LeMaker Team*/
static uint32_t pwm_period;
static int i2c_fd = -1;
static int spi_fd = -1;

/*end 2015.01.05*/

//
// Low level register access functions
//

// Function to return the pointers to the hardware register bases
uint32_t* bcm2835_regbase(uint8_t regbase)
{
    switch (regbase)
    {
	case BCM2835_REGBASE_GPIO:
	    return (uint32_t *)sunxi_gpio;
	case BCM2835_REGBASE_PWM:
	    return (uint32_t *)sunxi_pwm;
	case BCM2835_REGBASE_SPI0:
	    return (uint32_t *)sunxi_spi0;
	case BCM2835_REGBASE_IIC2:
		 return (uint32_t *)sunxi_iic2;

    }
    return (uint32_t *)MAP_FAILED;
}

void  bcm2835_set_debug(uint8_t d)
{
    debug = d;
}

// safe read from peripheral
uint32_t bcm2835_peri_read(volatile uint32_t* paddr)
{
    if (debug)
    {
        printf("bcm2835_peri_read  paddr %08X\n", (unsigned) paddr);
	return 0;
    }
    else
    {
	// Make sure we dont return the _last_ read which might get lost
	// if subsequent code changes to a different peripheral
	uint32_t ret = *paddr;
	*paddr; // Read without assigneing to an unused variable
	return ret;
    }
}

// read from peripheral without the read barrier
uint32_t bcm2835_peri_read_nb(volatile uint32_t* paddr)
{
    if (debug)
    {
	printf("bcm2835_peri_read_nb  paddr %08X\n", (unsigned) paddr);
	return 0;
    }
    else
    {
	return *paddr;
    }
}

// safe write to peripheral
void bcm2835_peri_write(volatile uint32_t* paddr, uint32_t value)
{
  if (debug)
    {
	printf("bcm2835_peri_write paddr %08X, value %08X\n", (unsigned) paddr, value);
    }
  else
    {
	// Make sure we don't rely on the first write, which may get
	// lost if the previous access was to a different peripheral.
	*paddr = value;
	*paddr = value;
    }
}

// write to peripheral without the write barrier
void bcm2835_peri_write_nb(volatile uint32_t* paddr, uint32_t value)
{
    if (debug)
    {
	printf("bcm2835_peri_write_nb paddr %08X, value %08X\n",
               (unsigned) paddr, value);
    }
    else
    {
	*paddr = value;
    }
}

// Set/clear only the bits in value covered by the mask
void bcm2835_peri_set_bits(volatile uint32_t* paddr, uint32_t value, uint32_t mask)
{
    uint32_t v = bcm2835_peri_read(paddr);
    v = (v & ~mask) | (value & mask);
    bcm2835_peri_write(paddr, v);
}

//
// Low level convenience functions
//

// Function select
// pin is a BCM2835 GPIO pin number NOT RPi pin number
//      There are 6 control registers, each control the functions of a block
//      of 10 pins.
//      Each control register has 10 sets of 3 bits per GPIO pin:
//
//      000 = GPIO Pin X is an input
//      001 = GPIO Pin X is an output
//      100 = GPIO Pin X takes alternate function 0
//      101 = GPIO Pin X takes alternate function 1
//      110 = GPIO Pin X takes alternate function 2
//      111 = GPIO Pin X takes alternate function 3
//      011 = GPIO Pin X takes alternate function 4
//      010 = GPIO Pin X takes alternate function 5
//
// So the 3 bits for port X are:
//      X / 10 + ((X % 10) * 3)

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode)
{
    // Function selects are 10 pins per 32 bit word, 3 bits per pin
	int32_t bpipin = BpiPin[pin];
	volatile uint32_t* paddr = sunxi_gpio + ((SUNXI_GPIO_CFG_OFFSET + (bpipin/32) * 0x24 +  ((bpipin%32)/8) *0x04) >> 2);
	uint8_t   shift = (bpipin % 8) * 4;
	uint32_t  mask = BCM2835_GPIO_FSEL_MASK << shift;
	uint32_t  value = mode << shift;
	if(debug)printf("bcm2835_gpio_fsel:before write:0x%x.\n",bcm2835_peri_read(paddr));
	bcm2835_peri_set_bits(paddr, value, mask);
	if(debug)printf("bcm2835_gpio_fsel:after write:0x%x.\n",bcm2835_peri_read(paddr));

}

/*Modify for BananaPro by LeMaker Team*/
// Set output pin
void bcm2835_gpio_set(uint8_t pin)
{

	int32_t bpipin = BpiPin[pin];

	volatile uint32_t* paddr = sunxi_gpio + ((SUNXI_GPIO_DAT_OFFSET +(bpipin/32) * 0x24) >> 2);
	uint8_t   shift = bpipin % 32;
	uint32_t  mask = 0x01 << shift;
	uint32_t  value = 1 << shift;
	bcm2835_peri_set_bits(paddr, value, mask);

}

/*Modify for BananaPro by LeMaker Team*/
// Clear output pin
void bcm2835_gpio_clr(uint8_t pin)
{
	int32_t bpipin = BpiPin[pin];
	volatile uint32_t* paddr = sunxi_gpio + ((SUNXI_GPIO_DAT_OFFSET +(bpipin/32) * 0x24) >> 2);
	uint8_t   shift = bpipin % 32;
	uint32_t  mask = 0x01 << shift;
	uint32_t  value = 0 << shift;
	bcm2835_peri_set_bits(paddr, value, mask);
}

/*Modify for BananaPro by LeMaker Team*/
// Set all output pins in the mask
void bcm2835_gpio_set_multi(uint32_t mask)
{
	uint8_t i = 0;
	for(i;i < GPIONUM;i++){
		if(mask & 0x01){
			if(BpiPin[i] > 0){
			int32_t bpipin = BpiPin[i];

			volatile uint32_t* paddr = sunxi_gpio + ((SUNXI_GPIO_DAT_OFFSET +(bpipin/32) * 0x24) >> 2);
			uint8_t   shift = bpipin % 32;
			uint32_t  mask_bit = 0x01 << shift;
			uint32_t  value = 1 << shift;
			bcm2835_peri_set_bits(paddr, value, mask_bit);
			}
		}
		mask = mask >>1;
	}
}

/*Modify for BananaPro by LeMaker Team*/
// Clear all output pins in the mask
void bcm2835_gpio_clr_multi(uint32_t mask)
{
	uint8_t i = 0;
	for(i;i < GPIONUM;i++){
		if(mask & 0x01){
			if(BpiPin[i] > 0){
			int32_t bpipin = BpiPin[i];
			//printf("pin = %d.\n",bpipin);
			volatile uint32_t* paddr = sunxi_gpio + ((SUNXI_GPIO_DAT_OFFSET +(bpipin/32) * 0x24) >> 2);
			uint8_t   shift = bpipin % 32;
			uint32_t  mask_bit = 0x01 << shift;
			uint32_t  value = 0 << shift;
			bcm2835_peri_set_bits(paddr, value, mask_bit);
			}
		}
		mask = mask >>1;
	}
}

/*Modify for BananaPro by LeMaker Team*/
// Read input pin
uint8_t bcm2835_gpio_lev(uint8_t pin)
{
	if(BpiPin[pin] > 0){
		int32_t bpipin = BpiPin[pin];
		volatile uint32_t* paddr = sunxi_gpio + ((SUNXI_GPIO_DAT_OFFSET +(bpipin/32) * 0x24) >> 2);
		uint8_t shift = bpipin % 32;
		uint32_t value = bcm2835_peri_read(paddr);
		return (value & (1 << shift)) ? HIGH : LOW;
	}
}

/*Modify for BananaPro by LeMaker Team*/
// See if an event detection bit is set
// Sigh cant support interrupts yet
uint8_t bcm2835_gpio_eds(uint8_t pin)
{
	uint8_t value,rising,i = 0;
	if(GpioDetectType[pin] == 0x01){//low detect
		value = bcm2835_gpio_lev(pin);
		if(value)
			return 0x00;
		else
			return 0x01;
	} else if(GpioDetectType[pin] == 0x02){//high detect
		value = bcm2835_gpio_lev(pin);
		if(value)
			return 0x01;
		else
			return 0x00;
	} else if(GpioDetectType[pin] == 0x04){//Rising edge detect
		if(ReadRisingCnt== 0){
			RisingModeValue = bcm2835_gpio_lev(pin);
			ReadRisingCnt = 1;
		}
		while(i++ < 200)
		{
			value = bcm2835_gpio_lev(pin);
			if(value == 0) {
				RisingModeValue = 0;
			}
			if((value == 1) && (RisingModeValue == 0)){
				ReadRisingCnt = 0;
				RisingModeValue = 1;
				//printf("Rising edge detect.\n");
				return 0x01;
			}
		}
		return 0x00;
	}else if(GpioDetectType[pin] == 0x08){//fall edge detect
		if(ReadFallCnt== 0){
			FallModeValue = bcm2835_gpio_lev(pin);
			ReadFallCnt = 1;
		}
		while(i++ < 200)
		{
			value = bcm2835_gpio_lev(pin);
			if(value == 1) {
				FallModeValue = 1;
			}
			if((value == 0) && (FallModeValue == 1)){
				ReadFallCnt = 0;
				//printf("Falling edge detect.\n");
				FallModeValue = 0;
				return 0x01;
			}
		}
		return 0x00;

	}
	else{
		printf("gpio detect type is wrong.\n");
	}
}

/*Modify for BananaPro by LeMaker Team*/
// Write a 1 to clear the bit in EDS
void bcm2835_gpio_set_eds(uint8_t pin)
{
	bcm2835_delay(500);
}

/*Modify for BananaPro by LeMaker Team*/
// Rising edge detect enable
void bcm2835_gpio_ren(uint8_t pin)
{
	GpioDetectType[pin] = 0x04;
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_gpio_clr_ren(uint8_t pin)
{
	GpioDetectType[pin] &= 0xFB;
}

/*Modify for BananaPro by LeMaker Team*/
// Falling edge detect enable
void bcm2835_gpio_fen(uint8_t pin)
{
	GpioDetectType[pin] = 0x08;
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_gpio_clr_fen(uint8_t pin)
{
	GpioDetectType[pin] &= 0xF7;
}

/*Modify for BananaPro by LeMaker Team*/
// High detect enable
void bcm2835_gpio_hen(uint8_t pin)
{
	GpioDetectType[pin] = 0x02;
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_gpio_clr_hen(uint8_t pin)
{
	GpioDetectType[pin] &= 0xFD;
}

/*Modify for BananaPro by LeMaker Team*/
// Low detect enable
void bcm2835_gpio_len(uint8_t pin)
{
	GpioDetectType[pin] = 0x01;
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_gpio_clr_len(uint8_t pin)
{
	GpioDetectType[pin] &= 0xFE;
}

/*Modify for BananaPro by LeMaker Team*/
// Async rising edge detect enable
void bcm2835_gpio_aren(uint8_t pin)
{
	GpioDetectType[pin] = 0x04;
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_gpio_clr_aren(uint8_t pin)
{
	GpioDetectType[pin] &= 0xFB;
}

/*Modify for BananaPro by LeMaker Team*/
// Async falling edge detect enable
void bcm2835_gpio_afen(uint8_t pin)
{
	GpioDetectType[pin] = 0x08;
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_gpio_clr_afen(uint8_t pin)
{
	GpioDetectType[pin] &= 0xF7;
}

/*Modify for BananaPro by LeMaker Team*/
// Set pullup/down
void bcm2835_gpio_pud(uint8_t pud)
{
	return;
}

/*Modify for BananaPro by LeMaker Team*/
// Pullup/down clock
// Clocks the value of pud into the GPIO pin
void bcm2835_gpio_pudclk(uint8_t pin, uint8_t on)
{
	return;
}

/*Modify for BananaPro by LeMaker Team*/
// Read GPIO pad behaviour for groups of GPIOs
uint32_t bcm2835_gpio_pad(uint8_t group)
{
	return 0;
}

/*Modify for BananaPro by LeMaker Team*/
// Set GPIO pad behaviour for groups of GPIOs
// powerup value for al pads is
// BCM2835_PAD_SLEW_RATE_UNLIMITED | BCM2835_PAD_HYSTERESIS_ENABLED | BCM2835_PAD_DRIVE_8mA
void bcm2835_gpio_set_pad(uint8_t group, uint32_t control)
{
	return;
}

// Some convenient arduino-like functions
// milliseconds
void bcm2835_delay(unsigned int millis)
{
    struct timespec sleeper;

    sleeper.tv_sec  = (time_t)(millis / 1000);
    sleeper.tv_nsec = (long)(millis % 1000) * 1000000;
    nanosleep(&sleeper, NULL);
}
/*Add for BananaPro by LeMaker Team*/
void delayMicrosecondsHard (unsigned int howLong)
{
  struct timeval tNow, tLong, tEnd ;

  gettimeofday (&tNow, NULL) ;
  tLong.tv_sec  = howLong / 1000000 ;
  tLong.tv_usec = howLong % 1000000 ;
  timeradd (&tNow, &tLong, &tEnd) ;

  while (timercmp (&tNow, &tEnd, <))
    gettimeofday (&tNow, NULL) ;
}

/*end 2014.01.05*/

/*Modify for BananaPro by LeMaker Team*/
// microseconds
void bcm2835_delayMicroseconds(uint64_t micros)
{
	struct timespec sleeper ;

	if (micros ==   0)
		return ;
		else if (micros  < 100)
		delayMicrosecondsHard (micros) ;
	else{
		sleeper.tv_sec  = (time_t)(micros / 1000000);
		sleeper.tv_nsec = (long)(micros % 1000) * 1000;
		nanosleep(&sleeper, NULL);
	}
}

//
// Higher level convenience functions
//

// Set the state of an output
void bcm2835_gpio_write(uint8_t pin, uint8_t on)
{
    if (on)
	bcm2835_gpio_set(pin);
    else
	bcm2835_gpio_clr(pin);
}

// Set the state of a all 32 outputs in the mask to on or off
void bcm2835_gpio_write_multi(uint32_t mask, uint8_t on)
{
    if (on)
	bcm2835_gpio_set_multi(mask);
    else
	bcm2835_gpio_clr_multi(mask);
}

// Set the state of a all 32 outputs in the mask to the values in value
void bcm2835_gpio_write_mask(uint32_t value, uint32_t mask)
{
    bcm2835_gpio_set_multi(value & mask);
    bcm2835_gpio_clr_multi((~value) & mask);
}

// Set the pullup/down resistor for a pin
//
// The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
// the respective GPIO pins. These registers must be used in conjunction with the GPPUD
// register to effect GPIO Pull-up/down changes. The following sequence of events is
// required:
// 1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
// to remove the current Pull-up/down)
// 2. Wait 150 cycles ? this provides the required set-up time for the control signal
// 3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
// modify ? NOTE only the pads which receive a clock will be modified, all others will
// retain their previous state.
// 4. Wait 150 cycles ? this provides the required hold time for the control signal
// 5. Write to GPPUD to remove the control signal
// 6. Write to GPPUDCLK0/1 to remove the clock
//
// RPi has P1-03 and P1-05 with 1k8 pullup resistor

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_gpio_set_pud(uint8_t pin, uint8_t pud)
{
    // Function selects are 10 pins per 32 bit word, 3 bits per pin
	if(BpiPin[pin] > 0){
		int32_t bpipin = BpiPin[pin];
		volatile uint32_t* paddr = sunxi_gpio + ((SUNXI_GPIO_PUL_OFFSET + (bpipin/32) * 0x24 +  ((bpipin%32)/16) *0x04) >> 2);
		uint8_t   shift = (bpipin % 16) * 2;
		uint32_t  mask = BCM2835_GPIO_PUD_MASK << shift;
		uint32_t  value = pud << shift;
//			printf("before write:0x%x.\n",bcm2835_peri_read(paddr));
//			printf("value = 0x%x,mask = 0x%x.\n",value,mask);
		bcm2835_peri_set_bits(paddr, value, mask);
//			printf("after write:0x%x.\n",bcm2835_peri_read(paddr));
	}
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_spi_begin(void)
{
	int ret;
	spi_fd = open ("/dev/spidev0.0", O_RDWR);

	if( spi_fd< 0){
		fprintf(stderr, "Unable to open /dev/spidev0.0: %s\n", strerror(errno)) ;
		return;
	}
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		printf("set SPI_IOC_WR_BITS_PER_WORD failed:%s\n", strerror(errno));
		return;
	}

	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1){
		fprintf (stderr, "set SPI_IOC_WR_MAX_SPEED_HZ failed:%s\n",strerror(errno)) ;
		return;
	}

	printf("open /dev/spidev0.0 ok..\n");
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_spi_end(void)
{
	// Set all the SPI0 pins back to input
	close(spi_fd);
}

void bcm2835_spi_setBitOrder(uint8_t order)
{
    // BCM2835_SPI_BIT_ORDER_MSBFIRST is the only one suported by SPI0
}

/*Modify for BananaPro by LeMaker Team*/
// defaults to 0, which means a divider of 65536.
// The divisor must be a power of 2. Odd numbers
// rounded down. The maximum SPI clock rate is
// of the APB clock
void bcm2835_spi_setClockDivider(uint32_t divider)
{
   int ret;
	speed = divider;

	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1){
		fprintf (stderr, "set SPI_IOC_WR_MAX_SPEED_HZ failed:%s\n",strerror(errno)) ;
		return;
	}
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_spi_setDataMode(uint8_t mode)
{
	int ret;
	if (mode < 0 ||mode > 3){
		printf("Waring: spi DataMode should be 0~3.\n");
		return;
	}
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		printf("ioctl: set SPI WR mode failed.\n");
		return;
	}
}

/*Modify for BananaPro by LeMaker Team*/
// Writes (and reads) a single byte to SPI
uint8_t bcm2835_spi_transfer(uint8_t value)
{
	struct spi_ioc_transfer spi ;
	uint8_t rx_value,tx_value,ret;
	tx_value = value;
	rx_value = value;

	spi.tx_buf        = (unsigned long)&tx_value;
	spi.rx_buf        = (unsigned long)&rx_value ;
	spi.len           = 1 ;
	spi.delay_usecs   = 0 ;
	spi.speed_hz      = speed ;
	spi.bits_per_word = 8 ;

	if(ret = ioctl (spi_fd, SPI_IOC_MESSAGE(1), &spi) < 0){
		fprintf (stderr, "transfer failed:%s\n", strerror(errno)) ;
		return ret;
	}
  else
  {
		return rx_value;
	}
}

/*Modify for BananaPro by LeMaker Team*/
// Writes (and reads) an number of bytes to SPI
void bcm2835_spi_transfernb(char* tbuf, char* rbuf, uint32_t len)
{
	uint8_t value,i,rx_data;
	i = 0;
	while(len > 0){
		value = tbuf[i];
		rx_data = bcm2835_spi_transfer(value);
		rbuf[i] = rx_data;
		len--;
		i++;
	}
}

/*Modify for BananaPro by LeMaker Team*/
// Writes an number of bytes to SPI
void bcm2835_spi_writenb(char* tbuf, uint32_t len)
{
		uint8_t value,i;
		i = 0;
		while(len > 0){
			value = tbuf[i];
			bcm2835_spi_transfer(value);
			len--;
			i++;
		}
}

// Writes (and reads) an number of bytes to SPI
// Read bytes are copied over onto the transmit buffer
void bcm2835_spi_transfern(char* buf, uint32_t len)
{
    bcm2835_spi_transfernb(buf, buf, len);
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_spi_chipSelect(uint8_t cs)
{
	volatile uint32_t *paddr;
	paddr = sunxi_spi0 + SUNXI_SPI0_CTRL/4;
	bcm2835_peri_set_bits(paddr,cs<<12,SUNXI_SPI0_CS_CS);

//CS ouput mode: 0-automatic output  1-manual output CS
	bcm2835_peri_set_bits(paddr,0<<16,SUNXI_SPIO_CS_CTRL);

}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_spi_setChipSelectPolarity(uint8_t cs, uint8_t active)
{
	volatile uint32_t* paddr ;
	if (active)
		active = 0;
	else
		active = 1;

	paddr = sunxi_spi0 + SUNXI_SPI0_CTRL/4;
	bcm2835_peri_set_bits(paddr,active<<4, SUNXI_SPIO_CS_POL);

}

/*Add for BananaPro by LeMaker Team*/
static inline int i2c_smbus_access (int fd, char rw, uint8_t command, int size, union i2c_smbus_data *data)
{
  struct i2c_smbus_ioctl_data args ;

  args.read_write = rw ;
  args.command    = command ;
  args.size       = size ;
  args.data       = data ;
  return ioctl (fd, I2C_SMBUS, &args) ;
}
/*end 2014.01.05*/

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_i2c_begin(void)
{
	if ((i2c_fd= open ("/dev/i2c-2", O_RDWR)) < 0)
	{
	fprintf(stderr, "bcm2835_i2c_begin: Unable to open /dev/i2c-2: %s\n",strerror(errno)) ;
	}
	printf("open dev/i2c-2 ok..\n");

}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_i2c_end(void)
{
	close(i2c_fd);
}

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_i2c_setSlaveAddress(uint8_t addr)
{
	// Set I2C Device Address
	if (ioctl (i2c_fd, I2C_SLAVE, addr) < 0){
		fprintf(stderr, "bcm2835_i2c_setSlaveAddress: set slave addr failed: %s\n",strerror(errno)) ;
	}
	printf("set slave addr ok.\n");
}

/*Modify for BananaPro by LeMaker Team*/
// defaults to 0x5dc, should result in a 166.666 kHz I2C clock frequency.
// The divisor must be a power of 2. Odd numbers
// rounded down.
void bcm2835_i2c_setClockDivider(uint16_t divider)
{
	uint32_t value;
  	volatile uint32_t* paddr;
	if(divider ==BCM2835_I2C_CLOCK_DIVIDER_148)
		value = 0x12;
	else if(divider == BCM2835_I2C_CLOCK_DIVIDER_150)
		value = 0x1A;
	else if(divider == BCM2835_I2C_CLOCK_DIVIDER_626)
		value = 0x2A;
	else
		value = 0x52;
	paddr = sunxi_iic2 + I2C2_CLK_OFFSET/4;
	bcm2835_peri_write(paddr,value);

}

/*Modify for BananaPro by LeMaker Team*/
// set I2C clock divider by means of a baudrate number
void bcm2835_i2c_set_baudrate(uint32_t baudrate)
{
	uint32_t divider;
	if(baudrate > 400000)
		baudrate = 400000;
	else if(baudrate < 100000)
		baudrate = 100000;
	if(baudrate > 340000)
		divider = BCM2835_I2C_CLOCK_DIVIDER_148;
	else if(baudrate > 240000)
		divider = BCM2835_I2C_CLOCK_DIVIDER_150;
	else if(baudrate > 150000)
		divider = BCM2835_I2C_CLOCK_DIVIDER_626;
	else divider = BCM2835_I2C_CLOCK_DIVIDER_2500;
	bcm2835_i2c_setClockDivider( (uint16_t)divider );
}

/*Modify for BananaPro by LeMaker Team*/
// Writes an number of bytes to I2C
uint8_t bcm2835_i2c_write(const char * buf, uint32_t len)
{
	int ret,i;

	union i2c_smbus_data data ;
	for(i = 0;i < len;i++)
	{
		data.byte = *(buf + i);
		//printf("write:data.byte = 0x%x\n",data.byte);
		ret = i2c_smbus_access (i2c_fd, I2C_SMBUS_WRITE, data.byte, I2C_SMBUS_BYTE, NULL);
		if(ret < 0){
			printf("i2c write failed:%s\n", strerror(errno));
			break;
		}
	}
	return ret;
}

/*Modify for BananaPro by LeMaker Team*/
// Read an number of bytes from I2C
uint8_t bcm2835_i2c_read(char* buf, uint32_t len)
{
	int ret,i;
	union i2c_smbus_data data ;
	for(i = 0;i < len;i++)
	{
		ret = i2c_smbus_access (i2c_fd, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
		if(ret < 0){
			printf("i2c read failed:%s\n",strerror(errno));
			break;
		}
		//printf("read:data.byte = 0x%x.\n",data.byte&0xff);
		*(buf + i) = data.byte&0xFF;
	}
	return ret;
}

/*Modify for BananaPro by LeMaker Team*/
// Read an number of bytes from I2C sending a repeated start after writing
// the required register. Only works if your device supports this mode
uint8_t bcm2835_i2c_read_register_rs(char* regaddr, char* buf, uint32_t len)
{
	int ret,i;
	union i2c_smbus_data data ;
	uint8_t reg = *regaddr;
	if (len == 1){
		ret = i2c_smbus_access (i2c_fd, I2C_SMBUS_READ, reg, I2C_SMBUS_BYTE, &data) ;
		if(ret < 0){
			printf("i2c read failed:%s\n",strerror(errno));
		}
		*buf = data.byte&0xFF;
	} else if (len == 2) {
		ret = i2c_smbus_access (i2c_fd, I2C_SMBUS_READ, reg, I2C_SMBUS_WORD_DATA, &data) ;
		if(ret < 0){
			printf("i2c read failed:%s\n",strerror(errno));
		}
		*buf = data.word&0xFFFF;
	} else {
		for(i = 0;i < len;i++)
		{
			ret = i2c_smbus_access (i2c_fd, I2C_SMBUS_READ, reg, I2C_SMBUS_BYTE, &data) ;
			if(ret < 0){
				printf("i2c read failed:%s\n",strerror(errno));
			}
			*(buf + i) = data.byte&0xFF;
		}
	}
	return ret;
}

/*Modify for BananaPro by LeMaker Team*/
// Sending an arbitrary number of bytes before issuing a repeated start
// (with no prior stop) and reading a response. Some devices require this behavior.
uint8_t bcm2835_i2c_write_read_rs(char* cmds, uint32_t cmds_len, char* buf, uint32_t buf_len)
{
	uint8_t ret;
	ret = bcm2835_i2c_write(cmds,cmds_len);
	ret = bcm2835_i2c_read(buf,buf_len);
	return ret;
}

/*Modify for BananaPro by LeMaker Team*/
// Read the System Timer Counter (64-bits)
uint64_t bcm2835_st_read(void)
{
	return 0;
}

/*Modify for BananaPro by LeMaker Team*/
// Delays for the specified number of microseconds with offset
void bcm2835_st_delay(uint64_t offset_micros, uint64_t micros)
{
	return;
}

// PWM

/*Modify for BananaPro by LeMaker Team*/
void bcm2835_pwm_set_clock(uint32_t divisor)
{
	volatile uint32_t* paddr;
	uint8_t value;
	paddr = sunxi_pwm + (SUNXI_PWM_CTRL_OFFSET /4);
	if((divisor == BCM2835_PWM_CLOCK_DIVIDER_32768) ||(divisor == BCM2835_PWM_CLOCK_DIVIDER_16384))
		value = 0x0C;
	else if((divisor == BCM2835_PWM_CLOCK_DIVIDER_8192) ||(divisor ==BCM2835_PWM_CLOCK_DIVIDER_4096))
		value = 0x0B;
	else if(divisor ==BCM2835_PWM_CLOCK_DIVIDER_2048)
		value = 0x0A;
	else if(divisor == BCM2835_PWM_CLOCK_DIVIDER_1024)
		value = 0x09;
	else if(divisor == BCM2835_PWM_CLOCK_DIVIDER_512)
		value = 0x08;
	else if(divisor == BCM2835_PWM_CLOCK_DIVIDER_256)
		value = 0x08;
	else if((divisor ==BCM2835_PWM_CLOCK_DIVIDER_128)||(divisor ==BCM2835_PWM_CLOCK_DIVIDER_64))
		value = 0x04;
	else if((divisor ==BCM2835_PWM_CLOCK_DIVIDER_32) ||(divisor ==BCM2835_PWM_CLOCK_DIVIDER_32))
		value = 0x03;
	else if((divisor ==BCM2835_PWM_CLOCK_DIVIDER_16) ||(divisor ==BCM2835_PWM_CLOCK_DIVIDER_8))
		value = 0x02;
	else if((divisor ==BCM2835_PWM_CLOCK_DIVIDER_4) ||(divisor ==BCM2835_PWM_CLOCK_DIVIDER_2))
		value = 0x01;
	else value = 0x00;
	bcm2835_peri_set_bits(paddr,value << 15,SUNXI_PWM_CLKDIV_MASK);

}

/*Modify for BananaPro by LeMaker Team*/
//
void bcm2835_pwm_set_mode(uint8_t channel, uint8_t markspace, uint8_t enabled)
{
	volatile uint32_t* paddr;
	if(markspace == 0)
		markspace = 1;
	else
		markspace = 0;
	paddr = sunxi_pwm + (SUNXI_PWM_CTRL_OFFSET /4);
	bcm2835_peri_set_bits(paddr,markspace<< 22,0x01<<22);

}

/*Modify for BananaPro by LeMaker Team*/
//compatible with the RPI PWM channel
//RPI:PWM0 <----> BPR:PWM1
void bcm2835_pwm_set_range(uint8_t channel, uint32_t range)
{
	volatile uint32_t* paddr;

	if(channel == 1)
		paddr = sunxi_pwm + (SUNXI_PWM_CH0_OFFSET /4);
	else if(channel == 0)
		paddr = sunxi_pwm + (SUNXI_PWM_CH1_OFFSET/4);
	if(range >= 0xFFFF)
		range = 0xFFFF;
	bcm2835_peri_set_bits(paddr,range<< 16,0xFFFF<<16);
 }

/*Modify for BananaPro by LeMaker Team*/
//compatible with the RPI PWM channel
//RPI:PWM0 <----> BPR:PWM1
void bcm2835_pwm_set_data(uint8_t channel, uint32_t data)
{
	uint32_t value,temp;
	volatile uint32_t* paddr;
	if(channel == 1)
		paddr = sunxi_pwm + (SUNXI_PWM_CH0_OFFSET /4);
	else if(channel == 0)
		paddr = sunxi_pwm + (SUNXI_PWM_CH1_OFFSET/4);
	if(data >= 0xFFFF)
		data = 0xFFFF;
	value = bcm2835_peri_read(paddr);
	value >>=16;
	if(data > value)
		data = value;
	bcm2835_peri_set_bits(paddr,data,0xFFFF);
	paddr = sunxi_pwm + (SUNXI_PWM_CTRL_OFFSET>>2);
	temp = bcm2835_peri_read(paddr);
	if(channel == 1)
		temp |= 0x230;
	else
		temp |= 0x380000;
	bcm2835_peri_write(paddr,temp);
}

// Allocate page-aligned memory.
void *malloc_aligned(size_t size)
{
    void *mem;
    errno = posix_memalign(&mem, BCM2835_PAGE_SIZE, size);
    return (errno ? NULL : mem);
}

// Map 'size' bytes starting at 'off' in file 'fd' to memory.
// Return mapped address on success, MAP_FAILED otherwise.
// On error print message.
static void *mapmem(const char *msg, size_t size, int fd, off_t off)
{
    void *map = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, off);
    if (MAP_FAILED == map)
	fprintf(stderr, "bcm2835_init: %s mmap failed: %s\n", msg, strerror(errno));
    return map;
}

static void unmapmem(void **pmem, size_t size)
{
    if (*pmem == MAP_FAILED) return;
    munmap(*pmem, size);
    *pmem = MAP_FAILED;
}
/*Add for BananaPro by LeMaker Team*/
static int get_cpuinfo_revision(void)
{
   FILE *fp;
   char buffer[1024];
   char hardware[1024];
	char *ret;

   if ((fp = fopen("/proc/cpuinfo", "r")) == NULL)
      return -1;

   while(!feof(fp))
	 {
      ret = fgets(buffer, sizeof(buffer) , fp);
      sscanf(buffer, "Hardware	: %s", hardware);
    	if (strcmp(hardware, "sun7i") == 0)
		{
			return 1;
		}
   }
   fclose(fp);
   return 0;
}

/*end 2015.01.05*/

 /*Modify for BananaPro by LeMaker Team*/
// Initialise this library.
int bcm2835_init(void)
{
	int memfd = -1;
	int ok = 0;
	uint32_t mmap_base;
	uint32_t mmap_seek;
	volatile uint32_t *sunxi_tmp;

	if (1 != get_cpuinfo_revision())
	{
		fprintf(stderr, "bcm3825_init: The current version only support the BananaPro !\n");
		goto exit;
	}
	 // Open the master /dev/memory device
	if ((memfd = open("/dev/mem", O_RDWR | O_SYNC) ) < 0)
	{
		fprintf(stderr, "bcm2835_init: Unable to open /dev/mem: %s\n",
		strerror(errno)) ;
		goto exit;
	}

	if (debug)
	{
		sunxi_gpio = (uint32_t*)SUNXI_GPIO_BASE;
		sunxi_pwm  = (uint32_t*)SUNXI_GPIO_PWM;
		sunxi_spi0 = (uint32_t*)SUNXI_SPI0_BASE;
		sunxi_iic2 = (uint32_t*)SUNXI_IIC2_BASE;
		return 1;
	}
	mmap_base = (SUNXI_GPIO_BASE & (~MAP_MASK));
	mmap_seek = (SUNXI_GPIO_BASE-mmap_base)>> 2;
	sunxi_gpio =  (uint32_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, mmap_base);
	if (MAP_FAILED == sunxi_gpio)
	{
		fprintf(stderr, "bcm2835_init: %s mmap failed: %s\n", "sunxi_gpio", strerror(errno));
		goto exit;
	}
	sunxi_tmp = sunxi_gpio;
	 /*gpio register base address*/
	sunxi_gpio += mmap_seek;

	mmap_base = (SUNXI_GPIO_PWM &(~MAP_MASK));
	mmap_seek = (SUNXI_GPIO_PWM - mmap_base) >> 2;
	/*pwm register base address*/
	sunxi_pwm =  sunxi_tmp + mmap_seek;

	/*spi register base address*/
	sunxi_spi0 =  (uint32_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, SUNXI_SPI0_BASE);
	if (MAP_FAILED == sunxi_spi0)
	{
		fprintf(stderr, "bcm2835_init: %s mmap failed: %s\n", "sunxi_spi0", strerror(errno));
		goto exit;
	}

	mmap_base = (SUNXI_IIC2_BASE &(~MAP_MASK));
	mmap_seek = (SUNXI_IIC2_BASE - mmap_base) >> 2;
	sunxi_iic2 = (uint32_t *)mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, mmap_base);
	if (MAP_FAILED == sunxi_iic2)
	{
		fprintf(stderr, "bcm2835_init: %s mmap failed: %s\n", "sunxi_iic2", strerror(errno));
		goto exit;
	}
	/*iic2 register base address*/
	sunxi_iic2 += mmap_seek;

	ok = 1;

	exit:
	    if (memfd >= 0)
	        close(memfd);

    if (!ok)
	bcm2835_close();

    return ok;
}

/*Modify for BananaPro by LeMaker Team*/
// Close this library and deallocate everything
int bcm2835_close(void)
{
	uint32_t mmap_base;
	uint32_t mmap_seek;
    if (debug) return 1; // Success

 	mmap_base = (SUNXI_GPIO_BASE & (~MAP_MASK));
	mmap_seek = (SUNXI_GPIO_BASE - mmap_base)>> 2;
	sunxi_gpio -= mmap_seek;
 	if(MAP_FAILED == sunxi_gpio) return 1;
	else
	{
		munmap((void *)sunxi_gpio, MAP_SIZE);
		sunxi_gpio = MAP_FAILED;
	}

	if(MAP_FAILED == sunxi_spi0) return 1;
	else
	{
		munmap((void *)sunxi_spi0, MAP_SIZE);
		sunxi_spi0 = MAP_FAILED;
	}

 	mmap_base = (SUNXI_IIC2_BASE & (~MAP_MASK));
	mmap_seek = (SUNXI_IIC2_BASE - mmap_base)>> 2;
	sunxi_iic2 -= mmap_seek;
	if(MAP_FAILED == sunxi_iic2) return 1;
	else
	{
		munmap((void *)sunxi_iic2, MAP_SIZE);
		sunxi_iic2 = MAP_FAILED;
	}

	return 1;
}

#ifdef BCM2835_TEST
// this is a simple test program that prints out what it will do rather than
// actually doing it
int main(int argc, char **argv)
{
    // Be non-destructive
    bcm2835_set_debug(1);

    if (!bcm2835_init())
	return 1;

    // Configure some GPIO pins fo some testing
    // Set RPI pin P1-11 to be an output
    bcm2835_gpio_fsel(RPI_GPIO_P1_11, BCM2835_GPIO_FSEL_OUTP);
    // Set RPI pin P1-15 to be an input
    bcm2835_gpio_fsel(RPI_GPIO_P1_15, BCM2835_GPIO_FSEL_INPT);
    //  with a pullup
    bcm2835_gpio_set_pud(RPI_GPIO_P1_15, BCM2835_GPIO_PUD_UP);
    // And a low detect enable
    bcm2835_gpio_len(RPI_GPIO_P1_15);
    // and input hysteresis disabled on GPIOs 0 to 27
    bcm2835_gpio_set_pad(BCM2835_PAD_GROUP_GPIO_0_27, BCM2835_PAD_SLEW_RATE_UNLIMITED|BCM2835_PAD_DRIVE_8mA);

#if 1
    // Blink
    while (1)
    {
	// Turn it on
	bcm2835_gpio_write(RPI_GPIO_P1_11, HIGH);

	// wait a bit
	bcm2835_delay(500);

	// turn it off
	bcm2835_gpio_write(RPI_GPIO_P1_11, LOW);

	// wait a bit
	bcm2835_delay(500);
    }
#endif

#if 0
    // Read input
    while (1)
    {
	// Read some data
	uint8_t value = bcm2835_gpio_lev(RPI_GPIO_P1_15);
	printf("read from pin 15: %d\n", value);

	// wait a bit
	bcm2835_delay(500);
    }
#endif

#if 0
    // Look for a low event detection
    // eds will be set whenever pin 15 goes low
    while (1)
    {
	if (bcm2835_gpio_eds(RPI_GPIO_P1_15))
	{
	    // Now clear the eds flag by setting it to 1
	    bcm2835_gpio_set_eds(RPI_GPIO_P1_15);
	    printf("low event detect for pin 15\n");
	}

	// wait a bit
	bcm2835_delay(500);
    }
#endif

    if (!bcm2835_close())
	return 1;

    return 0;
}
#endif
