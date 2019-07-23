/*
 * シリアルLEDテープを虹色に点灯させるよ
 * Make an LED-strip glitter like a rainbow.
 * (c) yoshiaki takata, 2017, 2018
 */

#include "serialled.h"
#include "pwmfifo.h"	/* delayMicroseconds */
#include "sin.h"

/* GPIO番号 */
#define LED_GPIO  18

/* LEDの個数 */
#define N_LED  60

/* frame per second */
#define FPS  60

/* 時刻 t, LED i の色を設定します Set the color of LED i at time t. */
void setColor(int t, int i)
{
  int s = 255;              /* saturation = max */
  int h = i * 360 / N_LED;  /* hue = 0..360 for each LED */
  h = (h + 360 - t % 360) % 360;  /* one cycle per 360/FPS seconds */

  int t0 = (t + 540 + 90) % (360 * 6) - 3 * i;
  t0 = (t0 > 540 ? t0 - 540 : 540 - t0) - 90;   /* triangle wave /\/\/\ */
  t0 = t0 < 0 ? 0 : t0 > 180 ? 180 : t0;        /* constrain(0, 180) */
  int c = cosdeg(t0);
  int v = (255 - c) / 2;          /* one cycle per 360 * 6/FPS seconds */
  int f1 = 165 + 90 * sindeg(t - 32 * i / 10 + 51) / 255;
  int f2 = 165 + 90 * sindeg(t - 70 * i / 10 + 13) / 255;
  int b = v * f1 / 255 * f2 / 255;

  /* 色を設定 (まだ送信しない) Set the color (but not transmit yet). */
  ledSetColorHSB(i, h, s, b);
}

int main()
{
  int t;

  /* セットアップするよ */
  if (ledSetup(LED_GPIO, N_LED) == -1) {
    return -1;
  }

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
    delayMicroseconds(1000 * 1000 / FPS);
  }

  /* 全部消しましょう */
  ledClearAll();

  return 0;
}
