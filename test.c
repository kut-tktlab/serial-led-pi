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

int main()
{
  int i;

  /* セットアップするよ */
  if (ledSetup(LED_GPIO, N_LED) == -1) {
    fprintf(stderr, "cannot setup serial led.\n");
    return -1;
  }

  /* 5回点灯するよ */
  for (i = 0; i < 5; i++) {
    int led;
    for (led = 0; led < N_LED; led++) {
      /* 各LEDがRGBの1色ずつ選ぶよ */
      int r = ((led + i) % 3 == 0 ? 255 : 0);
      int g = ((led + i) % 3 == 1 ? 255 : 0);
      int b = ((led + i) % 3 == 2 ? 255 : 0);
      /* 色を設定 (まだ送信しない) */
      ledSetColor(led, r, g, b);
    }
    /* 送信! */
    ledSend();

    /* 1秒待ってください */
    delayMicroseconds(1000 * 1000);
  }

  /* 全部消しましょう */
  ledClearAll();

  return 0;
}
