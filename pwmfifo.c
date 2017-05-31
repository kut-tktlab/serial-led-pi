/*
 * References:
 * - WiringPi - http://wiringpi.com
 * - RPIO     - https://github.com/metachris/RPIO
 * - PeterLemon/RaspberryPi - https://github.com/PeterLemon/RaspberryPi
 */

#include <stdio.h>
#include <unistd.h>	/* lseek */
#include <fcntl.h>
#include <stdint.h>	/* uint32_t, etc. */
#include <sys/mman.h>

#include "mailbox.h"
#include "pwmfifo.h"

#define SUCCESS  0
#define FAILURE  -1

/*
 * Control-register addresses & values
 * -----------------------------------
 */
#define PERIPHERAL_BASE	 0x3f000000	/* for RPi 2, 3 */

#define GPIO_BASE	(0x00200000 + PERIPHERAL_BASE)
#define GPFSEL0		(0x00 /4)
#define GPSET0		(0x1c /4)
#define GPCLR0		(0x28 /4)
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
#define DMA_ENABLE	(0xff0/4)
/* DMA_CS */
#define DMA_RESET	(1<<31)
#define DMA_INT		(1<<2)
#define DMA_END		(1<<1)
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
#define PAGE_SHIFT	12

/* Base addresses of control registers */
static volatile uint32_t *gpio;
static volatile uint32_t *clock;
static volatile uint32_t *pwm;
static volatile uint32_t *timer;
static volatile uint32_t *dma;

static int setupDma(void);
static void cleanupDma(void);

/*
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

/*
 * Set up this GPIO-manipulation module.
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

  gpio  = mmapControlRegs(fd, GPIO_BASE);
  clock = mmapControlRegs(fd, PWMCLK_BASE);
  pwm   = mmapControlRegs(fd, PWM_BASE);
  timer = mmapControlRegs(fd, TIMER_BASE);
  dma   = mmapControlRegs(fd, DMA_BASE);

  if (gpio == MAP_FAILED || clock == MAP_FAILED ||
      pwm  == MAP_FAILED || timer == MAP_FAILED || dma == MAP_FAILED)
  {
    return FAILURE;
  }

  if (setupDma() == FAILURE) {
    return FAILURE;
  }
  return SUCCESS;
}

/*
 * Clean up
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

/* GPIO number */
static int pwmfifoPin;

/* PWM mode */
enum PwmMode {
  PWM_MODE_NONE = 0,
  PWM_MODE_BALANCED = 1,
  PWM_MODE_MS = 2
};
static enum PwmMode pwmMode = PWM_MODE_NONE;

/*
 * Set the pin mode to PWM_OUTPUT.
 */
void pinModePwm(int pin)
{
  if (pin != 18 && pin != 19) {
    fprintf(stderr, "pinModePwm: only GPIO 18 and 19 are supported.\n");
    return;
  }
  /* Set the mode of the GPIO to alt5 */
  *(gpio + GPFSEL0 + pin/10) = GPFSEL_ALT5 << ((pin % 10) * 3);

  pwmfifoPin = pin;
}

/*
 * Set the PWM to the balanced mode.
 */
void pwmSetModeBalanced()
{
  pwmMode = PWM_MODE_BALANCED;
}

/*
 * Set the PWM to the mark:space mode.
 */
void pwmSetModeMS()
{
  pwmMode = PWM_MODE_MS;
}

/*
 * Set PWM clock divider.
 */
void pwmSetClock(unsigned int divider)
{
  unsigned int pwmctl;

  /* Set the PWM control register at first */
  if (pwmMode == PWM_MODE_NONE) {
    fprintf(stderr, "Warning: Please set the pwm mode before pwmSetClock()\n");
  }
  pwmctl = (pwmfifoPin == 18 ? (PWM1_USEFIFO | PWM1_ENABLE)
                             : (PWM2_USEFIFO | PWM2_ENABLE));
  if (pwmMode == PWM_MODE_MS) {
    pwmctl |= (pwmfifoPin == 18 ? PWM1_MSMODE : PWM2_MSMODE);
  }
  *(pwm + PWM_CTL) = pwmctl;

  /* Stop the clock */
  *(clock + PWMCLK_CTL) = PWMCLK_PASSWD | PWMCLK_SRC;
  delayMicroseconds(110);		/* cf. wiringPi.c */

  /* Wait while busy */
  while ((*(clock + PWMCLK_CTL) & 0x80) != 0) {
    delayMicroseconds(1);
  }

  /* Set the divider */
  *(clock + PWMCLK_DIV) = (PWMCLK_PASSWD | (divider << 12));
  *(clock + PWMCLK_CTL) = PWMCLK_PASSWD | PWMCLK_SRC | PWMCLK_ENABLE;
  delayMicroseconds(110);

  /* Set the PWM control register again */
  *(pwm + PWM_DMAC) = PWMDMAC_ENABLE | PWMDMAC_THRSHLD;
  *(pwm + PWM_CTL) = PWM_CLRFIFO;
  *(pwm + PWM_CTL) = pwmctl;
}

/*
 * Set PWM range. (Only supports GPIO #18 and #19)
 */
void pwmSetRange(unsigned int range)
{
  *(pwm + PWM_RNG1) = range;
  delayMicroseconds(10);
  *(pwm + PWM_RNG2) = range;
  delayMicroseconds(10);
}

/*
 * DMA
 * ----------------
 */

/* DMA channel used for PWM control (0..14) */
#define DMA_CHANNEL	0

/*
 * number of memory pages for DMA source and a control block:
 * ((24 * nLed + 40) * sizeof(uint32_t) + sizeof(dma_cb_t)) / PAGE_SIZE
 */
#define N_DMA_PAGES	2

/* mailbox & memory allocation */
static int mbox_handle;
static unsigned int mem_ref;	/* from mem_alloc() */
static unsigned int bus_addr;	/* from mem_lock() */
static uint8_t *virtaddr;	/* virtual addr of bus_addr */

#define DMA_CB_ADDR	((dma_cb_t *)virtaddr)
#define	DMA_SRC_ADDR	((uint32_t *)(virtaddr + sizeof(dma_cb_t)))
#define PWM_PHYS_BASE	(PWM_BASE - PERIPHERAL_BASE + 0x7e000000)
#define PWM_PHYS_FIFO	(PWM_PHYS_BASE + 0x18)

/* VC bus addr -> ARM physical addr */
#define BUS_TO_PHYS(x)  ((x) & 0x3fffffff)

/* ARM virtual addr -> ARM physical addr */
#define VIRT_TO_PHYS(x)	(bus_addr + ((uint8_t *)(x) - virtaddr))

/* a const used for mem_alloc */
#define MEM_FLAG	0x04		/* for RPi 2, 3 */

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

/*
 * Allocate memory pages of which physical addresses are known.
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
  delayMicroseconds(1000);

  /* Allocate memory */
  mem_ref = mem_alloc(mbox_handle, N_DMA_PAGES * PAGE_SIZE, PAGE_SIZE, MEM_FLAG);
  bus_addr = mem_lock(mbox_handle, mem_ref);
  virtaddr = mapmem(BUS_TO_PHYS(bus_addr), N_DMA_PAGES * PAGE_SIZE);

  printf("mem_ref %u\n", mem_ref);
  printf("bus_addr = %x\n", bus_addr);
  printf("virtaddr = %p\n", virtaddr);
  return SUCCESS;
}

/*
 * Set up the DMA controller
 */
static int setupDma()
{
  if (allocPagesForDma() == FAILURE) {
    return FAILURE;
  }

  if (dma == 0 || dma == MAP_FAILED) {
    fprintf(stderr, "DMA controller register is not mapped\n");
    return FAILURE;
  }
  dmaCh = dma + DMA_CHANNEL_INC * DMA_CHANNEL;

  *(dmaCh + DMA_CS) = DMA_RESET;
  delayMicroseconds(10);
  *(dmaCh + DMA_CS) = DMA_INT | DMA_END;  /* clear flags */

  return SUCCESS;
}

/*
 * Clean up the memories allocated for DMA
 */
static void cleanupDma()
{
  /* wait for the DMA to finish a current task */
  delayMicroseconds(3000);

  if (virtaddr != 0) {
    unmapmem(virtaddr, N_DMA_PAGES * PAGE_SIZE);
    mem_unlock(mbox_handle, mem_ref);
    mem_free(mbox_handle, mem_ref);
    printf("closing mailbox...\n");
    mbox_close(mbox_handle);
    printf("mailbox closed\n");
    virtaddr = 0;
  }
}

/*
 * Create a control block and run DMA.
 * The DMA channel automatically stops after submitting one sequence.
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
  cbp->next = 0;

  *(dmaCh + DMA_CONBLK_AD) = VIRT_TO_PHYS(cbp);
  *(dmaCh + DMA_DEBUG) = 7;			/* clear flags */
  *(dmaCh + DMA_CS) = 0x10880001;		/* go with mid priority */
}

/*
 * Write the contents of an array into PWM FIFO.
 */
void pwmWriteBlock(const unsigned char *array, int n)
{
  int i;
  uint32_t *srcp = DMA_SRC_ADDR;

  /* Move the data to a space of which the physical address is known */
  for (i = 0; i < n; i++) {
    srcp[i] = array[i];
  }

  /* Start DMA */
  startDma(n);
}

/*
 * Wait until the FIFO becomes empty.
 */
void pwmWaitFifoEmpty()
{
  /* wait while not empty */
  while ((*(pwm + PWM_STA) & 0x2) == 0) {
    delayMicroseconds(1);
  }
}

/*
 * delay
 * ----------------
 */

/* Wait for usec microseconds.  */
void delayMicroseconds(unsigned int usec)
{
  /* current + usec */
  uint32_t target = *(timer + TMCLO) + usec;

  /* wait until target */
  while (*(timer + TMCLO) < target) {}
}

