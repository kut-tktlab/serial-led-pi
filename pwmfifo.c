/*
 * pwmfifo.c:
 * a library for controlling PWM-related peripherals.
 *
 * This library is like a miniature of Gordon Henderson's WiringPi
 * and is specialized in feeding the PWM peripheral with data
 * sequentially using DMA.
 *
 * For Raspberry Pi 1, please set PI_VERSION to 1 before build
 * (though I only tested on RPi 3).
 *
 * References:
 * - GPIO and PWM
 *   - WiringPi - http://wiringpi.com
 * - DMA
 *   - RPIO     - https://github.com/metachris/RPIO
 *   - rpi-gpio-dma-demo      - https://github.com/hzeller/rpi-gpio-dma-demo
 *   - PeterLemon/RaspberryPi - https://github.com/PeterLemon/RaspberryPi
 *
 * pwmfifo.c
 * Copyright (c) 2017 Yoshiaki Takata
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>	/* uint32_t, etc. */
#include <unistd.h>	/* usleep */
#include <signal.h>	/* sigaction */
#include <sys/mman.h>

#include "mailbox.h"
#include "pwmfifo.h"

#ifndef PI_VERSION
# define PI_VERSION  2	/* RPi 2 and 3 */
#endif

#define SUCCESS  0
#define FAILURE  -1

/*
 * Control-register addresses & values
 * -----------------------------------
 */
#if PI_VERSION == 1
# define PERIPHERAL_BASE  0x20000000	/* for RPi 1 (not tested) */
#else
# define PERIPHERAL_BASE  0x3f000000	/* for RPi 2, 3 */
#endif

#define GPIO_BASE	(0x00200000 + PERIPHERAL_BASE)
#define GPFSEL0		(0x00 /4)
#define GPSET0		(0x1c /4)
#define GPCLR0		(0x28 /4)
#define GPFSEL_ALT0	4
#define GPFSEL_ALT5	2

#define PWMCLK_BASE	(0x00101000 + PERIPHERAL_BASE)
#define PWMCLK_CTL	(0xa0 /4)
#define PWMCLK_DIV	(0xa4 /4)
#define PWMCLK_PASSWD	0x5a000000
#define PWMCLK_ENABLE   0x10
#if NOT_USE_PLL
# define PWMCLK_SRC     0x1	/* oscillator */
#else
# define PWMCLK_SRC     0x6	/* PLLD */
#endif


#define PWM_BASE	(0x0020c000 + PERIPHERAL_BASE)
#define PWM_CTL		(0x00 /4)
#define PWM_STA		(0x04 /4)
#define PWM_DMAC	(0x08 /4)
#define PWM_RNG1	(0x10 /4)
#define PWM_DAT1	(0x14 /4)
#define PWM_FIF1	(0x18 /4)
#define PWM_RNG2	(0x20 /4)
#define PWM_DAT2	(0x24 /4)
#define PWM2_MSMODE	(1<<15)
#define PWM2_USEFIFO	(1<<13)
#define PWM2_ENABLE	(1<<8)
#define PWM1_MSMODE	(1<<7)
#define PWM_CLRFIFO	(1<<6)
#define PWM1_USEFIFO	(1<<5)
#define PWM1_ENABLE	(1<<0)
#define PWMDMAC_ENABLE	(1<<31)
#define PWMDMAC_THRSHLD	((7<<8)|(7<<0))

#define TIMER_BASE	(0x00003000 + PERIPHERAL_BASE)
#define TMCLO		(0x04 /4)

#define DMA_BASE	(0x00007000 + PERIPHERAL_BASE)
#define DMA_CS		(0x00 /4)
#define DMA_CONBLK_AD	(0x04 /4)
#define DMA_DEBUG	(0x20 /4)
#define DMA_CHANNEL_INC	(0x100/4)
/* DMA_CS */
#define DMA_RESET	(1<<31)
#define DMA_INT		(1<<2)
#define DMA_END		(1<<1)
#define DMA_ACTIVE	(1<<0)
#define DMA_PRIORITY(x)		((x)<<16)
#define DMA_PANIC_PRIORITY(x)	((x)<<20)
#define DMA_WAIT_FOR_OUTSTANDING_WRITES	(1<<28)
/* Transfer Information (TI) in a Control Block */
#define DMA_NO_WIDE_BURSTS	(1<<26)
#define DMA_PER_MAP(x)	((x)<<16)	/* peripheral map */
#define DMA_SRC_INC	(1<<8)
#define DMA_DEST_DREQ	(1<<6)
#define DMA_WAIT_RESP	(1<<3)

/*
 * Hardware setting up
 * -------------------
 */

/* paging size */
#define PAGE_SIZE	4096

/* Base addresses of control registers */
static volatile uint32_t *gpio;
static volatile uint32_t *clkman;
static volatile uint32_t *pwm;
static volatile uint32_t *timer;
static volatile uint32_t *dma;

static int setupDma(void);
static void cleanupDma(void);

/**
 * An aux function doing mmap
 */
static void *mmapControlRegs(int fd, off_t offset)
{
  void *p;
  p = mmap(0, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, offset);
  if (p == MAP_FAILED) {
    perror("mmap");
  }
  return p;
}

/**
 * Set up this GPIO-manipulation module.
 * \return 0 for success; -1 for failure.
 */
int setupGpio()
{
  int fd;

  /* Open /dev/mem (sudo required) */
  fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC);
  if (fd == -1) {
    perror("/dev/mem");
    return FAILURE;
  }

  gpio   = mmapControlRegs(fd, GPIO_BASE);
  clkman = mmapControlRegs(fd, PWMCLK_BASE);
  pwm    = mmapControlRegs(fd, PWM_BASE);
  timer  = mmapControlRegs(fd, TIMER_BASE);
  dma    = mmapControlRegs(fd, DMA_BASE);

  if (gpio == MAP_FAILED || clkman == MAP_FAILED ||
      pwm  == MAP_FAILED || timer  == MAP_FAILED || dma == MAP_FAILED)
  {
    return FAILURE;
  }

  if (setupDma() == FAILURE) {
    return FAILURE;
  }
  *(pwm + PWM_CTL) = 0;	/* reset PWM */
  return SUCCESS;
}

/**
 * Clean up.
 * \return 0 for success; -1 for failure.
 */
int cleanupGpio()
{
  cleanupDma();
  return SUCCESS;
}

/*
 * PWM
 * ----------------
 */

/* PWM channel (0 or 1) for a GPIO number */
#define PWM_CH(p)	((p) & 1)

/**
 * Set the pin mode to PWM_OUTPUT.
 * Only GPIO 12, 13, 18, and 19 are supported.
 * \param  pin  GPIO number.
 * \return 0 for success; -1 for failure.
 */
int pinModePwm(int pin)
{
  int alt;
  if (!(pin == 12 || pin == 13 || pin == 18 || pin == 19)) {
    fprintf(stderr, "pinModePwm: only GPIO 12,13,18,19 are supported.\n");
    return FAILURE;
  }
  /* Clear the mode of the GPIO */
  *(gpio + GPFSEL0 + pin/10) &= ~(7 << ((pin % 10) * 3));

  /* Set the mode of the GPIO to alt0 or alt5 */
  alt = (pin <= 13 ? GPFSEL_ALT0 : GPFSEL_ALT5);
  *(gpio + GPFSEL0 + pin/10) |= alt << ((pin % 10) * 3);

  return SUCCESS;
}

/**
 * Set the pin mode to PWM_OUTPUT with being feeded through FIFO.
 * Only GPIO 12, 13, 18, and 19 are supported.
 * \param pin  GPIO number.
 */
int pinModePwmFifo(int pin)
{
  if (pinModePwm(pin) == FAILURE) {
    return FAILURE;
  }
  *(pwm + PWM_CTL) &= ~(PWM1_USEFIFO | PWM2_USEFIFO);
  *(pwm + PWM_CTL) |= (PWM_CH(pin) == 0 ? PWM1_USEFIFO : PWM2_USEFIFO);
  return SUCCESS;
}

/**
 * Set the PWM to the balanced mode.
 * \param pin  GPIO number.
 */
void pwmSetModeBalanced(int pin)
{
  *(pwm + PWM_CTL) &= ~(PWM_CH(pin) == 0 ? PWM1_MSMODE : PWM2_MSMODE);
}

/**
 * Set the PWM to the mark:space mode.
 * \param pin  GPIO number.
 */
void pwmSetModeMS(int pin)
{
  *(pwm + PWM_CTL) |= (PWM_CH(pin) == 0 ? PWM1_MSMODE : PWM2_MSMODE);
}

/**
 * Set PWM clock divider.
 */
void pwmSetClock(unsigned int divider)
{
  unsigned int pwmctl;

  /* Enable the PWM channels at first */
  /* (I don't know why but unless doing so, PWM didn't work) */
  pwmctl = *(pwm + PWM_CTL);
  pwmctl |= PWM1_ENABLE | PWM2_ENABLE;
  *(pwm + PWM_CTL) = pwmctl;

  /* Stop the clock */
  *(clkman + PWMCLK_CTL) = PWMCLK_PASSWD | PWMCLK_SRC;
  usleep(110);		/* cf. wiringPi.c */

  /* Wait while busy */
  while ((*(clkman + PWMCLK_CTL) & 0x80) != 0) {
    usleep(1);
  }

  /* Set the divider */
  *(clkman + PWMCLK_DIV) = (PWMCLK_PASSWD | (divider << 12));
  *(clkman + PWMCLK_CTL) = PWMCLK_PASSWD | PWMCLK_SRC | PWMCLK_ENABLE;
  usleep(110);

  /* Set the PWM control register again */
  *(pwm + PWM_DMAC) = PWMDMAC_ENABLE | PWMDMAC_THRSHLD;
  *(pwm + PWM_CTL) = PWM_CLRFIFO;
  *(pwm + PWM_CTL) = pwmctl;
}

/**
 * Set PWM range.
 * \param pin   GPIO number.
 * \param range The number of the clock cycles of one PWM cycle.
 */
void pwmSetRange(int pin, unsigned int range)
{
  *(pwm + (PWM_CH(pin) == 0 ? PWM_RNG1 : PWM_RNG2)) = range;
}

/**
 * Write a single word to the PWM peripheral.
 * \param pin  GPIO number.
 * \param data Word to be written to the PWM peripheral.
 */
void pwmWrite(int pin, unsigned int data)
{
  *(pwm + (PWM_CH(pin) == 0 ? PWM_DAT1 : PWM_DAT2)) = data;
}

/*
 * DMA
 * ----------------
 * We use DMA for putting data sequentially to PWM.
 */

/* DMA channel used for PWM control (0..14) */
/* In rpi-gpio-dma-demo, it is said channel 5 is usually free */
#define DMA_CHANNEL	5

/*
 * number of memory pages for DMA source and a control block:
 * ((24 * nLed + 40) * sizeof(uint32_t) + sizeof(dma_cb_t)) / PAGE_SIZE
 */
#define N_DMA_PAGES	2

#define MAX_N_DMA_SAMPLES  ((int)(N_DMA_PAGES*PAGE_SIZE-sizeof(dma_cb_t))/4)

/* mailbox & memory allocation */
static int mbox_handle;
static unsigned int mem_ref;	/* from mem_alloc() */
static unsigned int bus_addr;	/* from mem_lock() */
static uint8_t *virtaddr;	/* virtual addr of bus_addr */

/* a const used for mem_alloc */
/* https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface */
#define MEM_FLAG_DIRECT		(1 << 2)
#define MEM_FLAG_COHERENT	(2 << 2)
#define MEM_FLAG_L1_NONALLOCATING	(MEM_FLAG_DIRECT | MEM_FLAG_COHERENT)

#define DMA_CB_ADDR	((dma_cb_t *)virtaddr)
#define	DMA_SRC_ADDR	((uint32_t *)(virtaddr + sizeof(dma_cb_t)))
#define PWM_PHYS_BASE	(PWM_BASE - PERIPHERAL_BASE + 0x7e000000)
#define PWM_PHYS_FIFO	(PWM_PHYS_BASE + 0x18)

/* VC bus addr -> ARM physical addr */
#define BUS_TO_PHYS(x)  ((x) & 0x3fffffff)

/* ARM virtual addr -> VC bus addr */
#define VIRT_TO_PHYS(x)	(bus_addr + ((uint8_t *)(x) - virtaddr))

/* DMA Control Block */
typedef struct {
  uint32_t info;	/* TI: transfer info */
  uint32_t src;		/* source address */
  uint32_t dst;		/* destination address */
  uint32_t length;	/* number of bytes */
  uint32_t stride;	/* 2D stride (dst<<16|src) */
  uint32_t next;	/* next control block */
  uint32_t pad[2];	/* reserved */
} dma_cb_t;

/* control register for a specified channel */
static volatile uint32_t *dmaCh;

/**
 * Allocate memory pages of which physical addresses are known.
 * \return 0 for success; -1 for failure.
 */
static int allocPagesForDma()
{
  if (virtaddr != 0) {
    fprintf(stderr, "DMA pages already allocated\n");
    return FAILURE;
  }

  /* Use mailbox to communicate with VC */
  mbox_handle = mbox_open();
  if (mbox_handle < 0) {
    fprintf(stderr, "Failed to open mailbox\n");
    return FAILURE;
  }
  usleep(1000);

  /* Allocate memory */
  mem_ref = mem_alloc(mbox_handle, N_DMA_PAGES * PAGE_SIZE, PAGE_SIZE,
                      MEM_FLAG_L1_NONALLOCATING);
  bus_addr = mem_lock(mbox_handle, mem_ref);
  virtaddr = mapmem(BUS_TO_PHYS(bus_addr), N_DMA_PAGES * PAGE_SIZE);

#if DEBUG
  printf("mem_ref %u\n", mem_ref);
  printf("bus_addr = %x\n", bus_addr);
  printf("virtaddr = %p\n", virtaddr);
#endif
  return SUCCESS;
}

/** Wait for the DMA channel to be inactive */
static void waitDmaInactive()
{
  while ((*(dmaCh + DMA_CS) & DMA_ACTIVE) != 0) {
    usleep(1);
  }
}

/**
 * Clean up the memories allocated for DMA
 */
static void cleanupDma()
{
  /* wait for the DMA to finish a current task */
  waitDmaInactive();

  if (virtaddr != 0) {
    unmapmem(virtaddr, N_DMA_PAGES * PAGE_SIZE);
    mem_unlock(mbox_handle, mem_ref);
    mem_free(mbox_handle, mem_ref);
    mbox_close(mbox_handle);
    virtaddr = 0;
#if DEBUG
    printf("mailbox closed\n");
#endif
  }
}

/** signal handler for cleaning up */
static void terminationHandler(int signum)
{
  cleanupDma();
  signum = signum;	/* suppress 'unused' warning */
}

/**
 * Set up the DMA controller
 */
static int setupDma()
{
  struct sigaction sa;

  /* allocate memory used for DMA */
  if (allocPagesForDma() == FAILURE) {
    return FAILURE;
  }

  /* initialize the DMA channel */
  if (dma == 0 || dma == MAP_FAILED) {
    fprintf(stderr, "DMA controller register is not mapped\n");
    return FAILURE;
  }
  dmaCh = dma + DMA_CHANNEL_INC * DMA_CHANNEL;

  *(dmaCh + DMA_CS) = DMA_RESET;
  usleep(10);
  *(dmaCh + DMA_CS) = DMA_INT | DMA_END;  /* clear flags */

  /* set a signal handler */
  sa.sa_handler = terminationHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT,  &sa, 0);
  sigaction(SIGHUP,  &sa, 0);
  sigaction(SIGTERM, &sa, 0);

  return SUCCESS;
}

/**
 * Create a control block and run DMA.
 * The DMA channel automatically stops after submitting one sequence.
 * \param n_samples  The number of words to be transmitted.
 */
static void startDma(int n_samples)
{
  dma_cb_t *cbp  = DMA_CB_ADDR;
  uint32_t *srcp = DMA_SRC_ADDR;

  if (virtaddr == 0) {
    fprintf(stderr, "Error: dma has not been set up\n");
    return;
  }

  cbp->info = DMA_NO_WIDE_BURSTS | DMA_WAIT_RESP |
              DMA_DEST_DREQ | DMA_PER_MAP(5) | DMA_SRC_INC;
  cbp->src = VIRT_TO_PHYS(srcp);
  cbp->dst = PWM_PHYS_FIFO;
  cbp->length = 4 * n_samples;
  cbp->stride = 0;
  cbp->next = 0;	/* no next control block */

  *(dmaCh + DMA_CONBLK_AD) = VIRT_TO_PHYS(cbp);
  *(dmaCh + DMA_DEBUG) = 7;			/* clear flags */
  *(dmaCh + DMA_CS) = DMA_WAIT_FOR_OUTSTANDING_WRITES |
                DMA_PANIC_PRIORITY(8) | DMA_PRIORITY(8) | /* mid priority */
                DMA_ACTIVE;				  /* go! */
}

/**
 * Write the contents of an array into PWM FIFO.
 * \param array  Bytes to be transmitted to the PWM peripheral.
 * \param n      The number of bytes in `array`.
 */
void pwmWriteBlock(const unsigned char *array, int n)
{
  int i;
  uint32_t *srcp = DMA_SRC_ADDR;

  if (n > MAX_N_DMA_SAMPLES) {
    fprintf(stderr, "Error: n_samples must be <= %d\n", MAX_N_DMA_SAMPLES);
    return;
  }

  /* If the DMA channel is active, wait for it to finish */
  waitDmaInactive();

  /* Move the data to a space whose physical address is known */
  for (i = 0; i < n; i++) {
    srcp[i] = array[i];
  }

  /* Start DMA */
  startDma(n);
}

/**
 * Wait until the FIFO becomes empty.
 */
void pwmWaitFifoEmpty()
{
  /* wait while not empty */
  while ((*(pwm + PWM_STA) & 0x2) == 0) {
    usleep(1);
  }
}
