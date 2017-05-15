#include "pwmfifo.h"

/*
 * *** GPIO ***
 */

/*
 * Addresses in the SoC
 */
#define PERIPHERAL_BASE	((void*)0x3f000000)

#define GPIO_BASE	(0x00200000 + PERIPHERAL_BASE)
#define GPFSEL0		(0x00 /sizeof(uint32_t))
#define GPSET0		(0x1c /sizeof(uint32_t))
#define GPCLR0		(0x28 /sizeof(uint32_t))
#define GPFSEL_ALT5	2

#define PWMCLK_BASE	(0x00101000 + PERIPHERAL_BASE)
#define PWMCLK_CTL	(0xa0 /sizeof(uint32_t))
#define PWMCLK_DIV	(0xa4 /sizeof(uint32_t))
#define PWMCLK_PASSWD	0x5a000000

#define PWM_BASE	(0x0020c000 + PERIPHERAL_BASE)
#define PWM_CTL		(0x00 /sizeof(uint32_t))
#define PWM_STA		(0x04 /sizeof(uint32_t))
#define PWM_RNG1	(0x10 /sizeof(uint32_t))
#define PWM_DAT1	(0x14 /sizeof(uint32_t))
#define PWM_FIF1	(0x18 /sizeof(uint32_t))
#define PWM_RNG2	(0x20 /sizeof(uint32_t))
#define PWM_DAT2	(0x24 /sizeof(uint32_t))
#define PWM2_MSMODE	(1<<15)
#define PWM2_USEFIFO	(1<<13)
#define PWM2_ENABLE	(1<<8)
#define PWM1_MSMODE	(1<<7)
#define PWM_CLRFIFO	(1<<6)
#define PWM1_USEFIFO	(1<<5)
#define PWM1_ENABLE	(1<<0)

#define TIMER_BASE	(0x00003000 + PERIPHERAL_BASE)
#define TMCLO		(0x04 /sizeof(uint32_t))

/* paging size */
#define BLOCK_SIZE	(4 * 1024)

typedef unsigned int uint32_t;

/* Base addresses of control registers */
static volatile uint32_t *gpio, *clock, *pwm, *timer;

/* GPIO number */
static int pwmfifo_pin;

/* PWM mode */
enum PwmMode {
  PWM_MODE_NONE = 0, PWM_MODE_BALANCED = 1, PWM_MODE_MS = 2
};
static enum PwmMode pwmMode = PWM_MODE_NONE;

#define mmapControlRegs(fd, offset)  (offset)
#define MAP_FAILED  0

/*
 * Set up this GPIO-manipulation module.
 * \return 0 for success; -1 otherwise.
 */
int setupGpio()
{
  gpio  = mmapControlRegs(fd, GPIO_BASE);
  clock = mmapControlRegs(fd, PWMCLK_BASE);
  pwm   = mmapControlRegs(fd, PWM_BASE);
  timer = mmapControlRegs(fd, TIMER_BASE);

  if (gpio == MAP_FAILED || clock == MAP_FAILED ||
      pwm  == MAP_FAILED || timer == MAP_FAILED)
  {
    return -1;
  }
  return 0;
}

/*
 * *** PWM ***
 */

/* Set the pin mode to PWM_OUTPUT.  */
void pinModePwm(int pin)
{
  if (pin != 18 && pin != 19) {
    return;
  }
  /* Set the mode of the GPIO to alt5 */
  *(gpio + GPFSEL0 + pin/10) = GPFSEL_ALT5 << ((pin % 10) * 3);

  pwmfifo_pin = pin;
}

/* Set the PWM to the balanced mode.  */
void pwmSetModeBalanced()
{
  pwmMode = PWM_MODE_BALANCED;
}

/* Set the PWM to the mark:space mode.  */
void pwmSetModeMS()
{
  pwmMode = PWM_MODE_MS;
}

/* Set PWM clock divider.  */
void pwmSetClock(unsigned int divider)
{
  unsigned int pwmctl;

  /* Set the PWM control register at first */
  pwmctl = (pwmfifo_pin == 18 ? (PWM1_USEFIFO | PWM1_ENABLE)
                              : (PWM2_USEFIFO | PWM2_ENABLE));
  if (pwmMode == PWM_MODE_MS) {
    pwmctl |= (pwmfifo_pin == 18 ? PWM1_MSMODE : PWM2_MSMODE);
  }
  *(pwm + PWM_CTL) = pwmctl;

  /* Stop the clock */
  *(clock + PWMCLK_CTL) = PWMCLK_PASSWD|0x21;	/* enable=0, osc */
  delayMicroseconds(110);		/* cf. wiringPi.c */

  /* Wait while busy */
  while ((*(clock + PWMCLK_CTL) & 0x80) != 0) {}
  delayMicroseconds(1);

  /* Set the divider */
  *(clock + PWMCLK_DIV) = (PWMCLK_PASSWD | (divider << 12));
  *(clock + PWMCLK_CTL) = PWMCLK_PASSWD|0x211;	/* enable=1, osc */

  /* Set the PWM control register again */
  *(pwm + PWM_CTL) = PWM_CLRFIFO;
  *(pwm + PWM_CTL) = pwmctl;
}

/* Set PWM range. (Only supports GPIO #18 and #19) */
void pwmSetRange(unsigned int range)
{
  *(pwm + PWM_RNG1) = range;
  delayMicroseconds(10);
  *(pwm + PWM_RNG2) = range;
  delayMicroseconds(10);
}

/* Write a byte into PWM FIFO. */
void pwmWriteFifo(unsigned int byte)
{
    uint32_t status;

    /* wait while full */
    while (((status = *(pwm + PWM_STA)) & 0x1) != 0) {
      uint32_t err = (status & 0x1fc);
      if (err != 0) { *(pwm + PWM_STA) = err; }
    }
    *(pwm + PWM_FIF1) = byte;
}

/* Wait until the FIFO becomes empty. */
void pwmWaitFifoEmpty()
{
  /* wait while not empty */
  while ((*(pwm + PWM_STA) & 0x2) == 0) {}
}

/*
 * *** delay ***
 */

/* Wait for usec microseconds.  */
void delayMicroseconds(unsigned int usec)
{
  /* current + usec */
  uint32_t target = *(timer + TMCLO) + usec;

  /* wait until target */
  while (*(timer + TMCLO) < target) {}
}

