/**
 * Data receiver for blockly-for-led.
 * cf. https://github.com/kut-tktlab/blockly-for-led
 *
 * Keep on waiting for data coming thru a named pipe,
 * and when receiving a data, call the functions in
 * serialled.so.
 *
 * Data format:
 * #xxxxxx#xxxxxx...#xxxxxx\n
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "serialled.h"

#define LED_FIFO  "/tmp/bky-led-fifo"
#define DEFAULT_GPIO_PIN  18
#define DEFAULT_N_LED     12
#define BUF_SIZE  1024  /* must > 7 * N_LED */

#define DECODE_1HEX(p) (*(p) >= 'a' ? *(p) - 'a' + 10 : \
                        *(p) >= 'A' ? *(p) - 'A' + 10 : *(p) - '0')
#define DECODE_HEX(p)  ((DECODE_1HEX(p) << 4) | DECODE_1HEX((p) + 1))

int main(int argc, char **argv)
{
  int fd;
  static char buf[BUF_SIZE];  
  unsigned int nbyte;
  int gpioPin = DEFAULT_GPIO_PIN;
  int nLed = DEFAULT_N_LED;
  int c;

  /* Parse command line arguments */
  opterr = 0;
  while ((c = getopt(argc, argv, "g:n:")) != -1) {
    switch (c) {
    case 'g': /* GPIO pin */
      gpioPin = atoi(optarg);
      break;
    case 'n': /* number of LEDs */
      nLed = atoi(optarg);
      break;
    case '?':
      if (optopt == 'g' || optopt == 'n') {
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      } else {
        fprintf(stderr, "Unknown option: -%c\n", optopt);
      }
      fprintf(stderr, "Usage: %s [-g gpio_pin] [-n n_led]\n", argv[0]);
      return 1;
    default:
      abort();
    }
  }

  printf("Starting the blockly-receiver...\n");
  printf("gpioPin=%d, nLed=%d\n", gpioPin, nLed);

  /* Set up */
  if (ledSetup(gpioPin, nLed) == -1) {
    fprintf(stderr, "cannot setup serial led.\n");
    return 1;
  }

  fd = open(LED_FIFO, O_RDONLY);
  if (fd == -1) {
    perror(LED_FIFO);
    exit(1);
  }

  /* Receive one chunk of data */
  while ((nbyte = read(fd, buf, sizeof(buf))) > 0) {
    int n = (nbyte - 1) / 7;  /* 7 == length of "#xxxxxx" */
    int i;
    char *p = buf;
    if (n > nLed) { n = nLed; }
    for (i = 0; i < n; i++, p += 7) {
      ledSetColor(i, DECODE_HEX(p+1), DECODE_HEX(p+3), DECODE_HEX(p+5));
    }
    ledSend();
  }

  /* Clean up */
  close(fd);
  ledCleanup();
  return 0;
}
