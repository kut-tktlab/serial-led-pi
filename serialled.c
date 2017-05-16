/*
 * シリアルLEDテープを点灯させるライブラリだよ
 */

#include "serialled.h"
#include "pwmfifo.h"

/* PWMの分周数 */
#define PWM_CLOCK_DIV	2	/* 1だとうまく動かなかったよ */

/*
 * LEDに送る信号 (pwmディジタル信号)
 * 0: High 0.4us+-150ns -> Low 0.85us+-150ns
 * 1: High 0.8us+-150ns -> Low 0.45us+-150ns
 * RET: Low >=50us
 * 19.2MHz, PWM_CLOCK_DIV==2 のとき1クロック104ns (あまり余裕ない)
 */
#define CLOCK_MHZ  (19.2 / PWM_CLOCK_DIV)
#define PWM_RANGE ((int)((.4 + .85) * CLOCK_MHZ + .5)) /* mark + space (波長) */
#define ZERO      ((int)(.4 * CLOCK_MHZ + .5))	    /* 0のmark長 */
#define ONE       ((int)(.8 * CLOCK_MHZ + .5))	    /* 1のmark長 */
#define SPACE     ((int)(50 / (.4 + .85)) + 1)      /* RETがrange何個分か */

/* R,G,Bの値のビット幅 */
#define RGB_BITS  8
#define RGB_MAX   ((1 << RGB_BITS) - 1)

/* LED数 */
static int nLed;

/* 各LEDの色の配列 */
static unsigned int ledColor[MAX_N_LED];


/*
 * セットアップするよ
 * \param gpioPin  LEDテープのGPIO番号
 * \param n        LEDの個数
 * \return  成功時 0, 失敗時 -1
 */
int ledSetup(int gpioPin, int n)
{
  if (setupGpio() == -1) {
    return -1;
  }
  pinModePwm(gpioPin);
  pwmSetModeMS();	/* mark:spaceモードだよ */
  pwmSetClock(PWM_CLOCK_DIV);
  pwmSetRange(PWM_RANGE);

  if (n < 0) { return -1; }
  if (n > MAX_N_LED) { return -1; }
  nLed = n;
  return 0;
}

/*
 * 1素子の色を設定 (まだ送信しない)
 * \param led  素子番号 (0〜)
 * \param r    赤の輝度 (0〜255)
 * \param g    緑　〃
 * \param b    青　〃
 */
void ledSetColor(int led, int r, int g, int b)
{
  if (led < 0 || led >= nLed) { return; }
  if (r < 0) { r = 0; }
  if (g < 0) { g = 0; }
  if (b < 0) { b = 0; }
  if (r > RGB_MAX) { r = RGB_MAX; }
  if (g > RGB_MAX) { g = RGB_MAX; }
  if (b > RGB_MAX) { b = RGB_MAX; }
  /* シリアルLEDテープ http://ssci.to/1399 はG, R, Bの順 */
  ledColor[led] = ((g << (2 * RGB_BITS)) | (r << RGB_BITS) | b);
}

/*
 * 色情報を送信!
 */
void ledSend()
{
  int i;
  for (i = 0; i < nLed; i++) {
    int col = ledColor[i];
    int mask = (1 << (3 * RGB_BITS - 1));
    for (; mask != 0; mask >>= 1) {
      pwmWriteFifo((col & mask) ? ONE : ZERO);
    }
  }
  /* RET信号 */
  for (i = 0; i < SPACE; i++) {
    pwmWriteFifo(0);
  }
  /* Wait until the fifo becomes empty */
  pwmWaitFifoEmpty();
}

/*
 * 全部消しましょう
 */
void ledClearAll()
{
  int led;
  for (led = 0; led < nLed; led++) {
    ledSetColor(led, 0, 0, 0);
  }
  ledSend();
}

/*
 * 1素子の色をHSBで設定 (まだ送信しない)
 * \param led  素子番号 (0〜)
 * \param h    色相 (0〜359)
 * \param s    彩度 (0〜255)
 * \param v    輝度　〃
 */
void ledSetColorHSB(int led, int h, int s, int v)
{
  int hgroup;
  double f, ss;
  int p, q, t, r, g, b;

  /* 各値を範囲に収めます */
  if (h < 0)   { h = 0; }
  if (h > 359) { h = 359; }
  if (s < 0)   { s = 0; }
  if (v < 0)   { v = 0; }
  if (s > RGB_MAX) { s = RGB_MAX; }
  if (v > RGB_MAX) { v = RGB_MAX; }

  /* 60度ごとのグループ(0..5)に分けます */
  hgroup = (h / 60) % 6;

  /* rgbに変換します */
  f  = (double)(h % 60) / 60;
  ss = (double)s / RGB_MAX;
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
