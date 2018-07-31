/*
 * シリアルLEDテープを虹色に点灯させるよ
 * Make an LED-strip glitter like a rainbow.
 * (c) yoshiaki takata, 2017
 */

#include <stdio.h>
#include <unistd.h>	/* usleep */

#include "serialled.h"

/* GPIO番号 */
#define LED_GPIO  18

/* LEDの個数 */
#define N_LED  60

/* frame per second */
#define FPS  60

int main()
{
  int i;

  int nc = nice(-20);
#if DEBUG
  fprintf(stderr, "set nice to %d\n", nc);
#endif
  nc += 0;  /* to suppress warnings */

  /* セットアップするよ */
  if (ledSetup(LED_GPIO, N_LED) == -1) {
    fprintf(stderr, "cannot setup serial led.\n");
    return -1;
  }

  /* wait 5 sec */
  usleep(1000 * 1000 * 5);

  /* 30秒間点灯するよ Glitter for 30 seconds. */
  for (i = 0; i < 30 * FPS; i++) {
    int led;
    for (led = 0; led < N_LED; led++) {
      int h = (led * (360 / N_LED) + i * 2) % 360;
      int s = 255;
      int b = 128;
      /* 色を設定 (まだ送信しない) */
      ledSetColorHSB(led, h, s, b);
    }
    /* 送信! */
    ledSend();

    /* 次のフレームまで待ってください Wait until the next frame. */
    usleep(1000 * 1000 / FPS);
  }

  /* 全部消灯します Turn off all LEDs. */
  ledClearAll();

  /* 後片付け */
  ledCleanup();

  return 0;
}
