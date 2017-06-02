/*
 * シリアルLEDテープを点灯させるライブラリだよ
 */

#include "serialled.h"
#include "pwmfifo.h"

/* PWMの分周数 */
#define PWM_CLOCK_DIV	25	/* 500MHz (PLLD) / 25 == 20MHz */

/*
 * LEDに送る信号 (pwmディジタル信号)
 * (シリアルLEDテープ http://ssci.to/1400 の場合)
 * 0: High 0.4us+-150ns -> Low 0.85us+-150ns
 * 1: High 0.8us+-150ns -> Low 0.45us+-150ns
 * RET: Low >=50us
 */
#define PWM_RANGE 25	/* 1.25us 1波長 */
#define ZERO      8	/* 0.4us  0のmark長 */
#define ONE       16	/* 0.8us  1のmark長 */
#define SPACE     40	/* RET(50us)が何波長(1.25us)分か */

/*
 * R,G,Bの送信順序
 * http://ssci.to/1400 の場合はG,R,Bの順
 */
#define COLOR_ORDER	ORDER_GRB
#define ORDER_GRB	0
#define ORDER_RGB	1

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
 * 後片付け
 */
void ledCleanup()
{
  ledClearAll();  /* 消灯! */
  cleanupGpio();
}

#define PACK_COLOR(h,m,l)  (((h)<<(2*RGB_BITS))|((m)<<RGB_BITS)|(l))

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

#if COLOR_ORDER == ORDER_GRB
  ledColor[led] = PACK_COLOR(g, r, b);
#else
  ledColor[led] = PACK_COLOR(r, g, b);
#endif
}

/*
 * 色情報を送信!
 */
void ledSend()
{
  static unsigned char buf[MAX_N_LED * 3 * RGB_BITS + SPACE];
  int i, j;
  for (i = 0; i < nLed; i++) {
    int col = ledColor[i];
    int mask = (1 << (3 * RGB_BITS - 1));
    for (j = 0; j < 3 * RGB_BITS; j++) {
      buf[i * 3 * RGB_BITS + j] = ((col & mask) ? ONE : ZERO);
      mask >>= 1;
    }
  }
  /* RET信号 */
  for (i = 0; i < SPACE; i++) {
    buf[nLed * 3 * RGB_BITS + i] = 0;
  }
  pwmWriteBlock(buf, nLed * 3 * RGB_BITS + SPACE);

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
