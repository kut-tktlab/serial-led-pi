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

/*
 * HSBカラーモデルで色を指定するよ
 * hは0..359, sとbは0..255
 */
void ledSetColorHSB(int led, int h, int s, int v)
{
  int hgroup;
  double f, ss;
  int p, q, t, r, g, b;

  /* 各値を範囲に収めます */
  if (h < 0)   { h = 0;   }
  if (h > 359) { h = 359; }
  if (s < 0)   { s = 0;   }
  if (s > 255) { s = 255; }
  if (v < 0)   { v = 0;   }
  if (v > 255) { v = 255; }

  /* 60度ごとのグループ(0..5)に分けます */
  hgroup = (h / 60) % 6;

  /* rgbに変換します */
  f  = (h % 60) / 60.0;
  ss = s / 255.0;
  p  = (int)(v * (1.0 - ss));
  q  = (int)(v * (1.0 - f * ss));
  t  = (int)(v * (1.0 - (1.0 - f) * ss));
  switch (hgroup) {
  case 0: r = v; g = t; b = p; break;
  case 1: r = q; g = v; b = p; break;
  case 2: r = p; g = v; b = t; break;
  case 3: r = p; g = q; b = v; break;
  case 4: r = t; g = p; b = v; break;
  case 5: r = v; g = p; b = q; break;
  }

  /* 変換結果を使います */
  ledSetColor(led, r, g, b);
}

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

    /* 少し待ってください */
    delayMicroseconds(1000 * 1000 / FPS);
  }

  /* 全部消しましょう */
  ledClearAll();

  return 0;
}
