/*
 * シリアルLEDテープを点灯させるよ
 */

#include <stdio.h>

#include "serialled.h"
#include "pwmfifo.h"	/* delayMicroseconds */

/* GPIO番号 */
#define LED_GPIO  18

/* LEDの個数 */
#define N_LED  10

/* frame per second */
#define FPS  30

int main()
{
  int i;

  /* セットアップするよ */
  if (ledSetup(LED_GPIO, N_LED) == -1) {
    fprintf(stderr, "cannot setup serial led.\n");
    return -1;
  }

  /* 30秒間点灯するよ */
  for (i = 0; i < 30 * FPS; i++) {
    int led;
    for (led = 0; led < N_LED; led++) {
      int h = (led * (360 / N_LED) + i * 2) % 360;
      /* 色を設定 (まだ送信しない) */
      ledSetColorHSB(led, h, 255, 128);
    }
    /* 送信! */
    ledSend();

    /* 次のフレームまで待ちましょう */
    delayMicroseconds(1000 * 1000 / FPS);
  }

  /* 全部消しましょう */
  ledClearAll();

  return 0;
}
