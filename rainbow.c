/*
 * シリアルLEDテープを虹色に点灯させるよ
 * Make an LED-strip glitter like a rainbow.
 * (c) yoshiaki takata, 2017, 2018
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

/* 時刻 t, LED i の色を設定します Set the color of LED i at time t. */
void setColor(int t, int i)
{
  int h = (i * (360 / N_LED) + t * 2) % 360;
  int s = 255;
  int b = 128;

  /* 色を設定 (まだ送信しない) Set the color (but not transmit yet). */
  ledSetColorHSB(i, h, s, b);
}

int main()
{
  int t;

  /* 実行優先度を上げるよ Increase the process priority. */
  int nc = nice(-20);
#if DEBUG
  fprintf(stderr, "set nice to %d\n", nc);
#endif
  nc += 0;  /* to suppress warnings */

  /* セットアップするよ (sudoしないとここで止まる)
   * Setting up (if not sudo-ed, this script stops here) */
  if (ledSetup(LED_GPIO, N_LED) == -1) {
    fprintf(stderr, "cannot setup serial led.\n");
    return -1;
  }

  /* wait 5 sec */
  usleep(1000 * 1000 * 5);

  /* 6時間点灯するよ Glitter for 6 hours. */
  for (t = 0; t < 6 * 3600 * FPS; t++) {
    int led;
    for (led = 0; led < N_LED; led++) {
      /* 色を設定 (まだ送信しない) Set the color (but not transmit yet). */
      setColor(t, led);
    }
    /* 送信! Transmit the color data! */
    ledSend();

    /* 次のフレームまで待ってください Wait until the next frame. */
    usleep(1000 * 1000 / FPS);
  }

  /* 全部消灯します Turn off all LEDs. */
  ledClearAll();

  /* 後片付けしてね Please call ledCleanup before exit. */
  ledCleanup();

  return 0;
}
