/*
 * serialled.c:
 * A library for controlling a LED strip like Adafruit NeoPixels.
 *
 * Copyright (c) 2017 Yoshiaki Takata
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "serialled.h"
#include "pwmfifo.h"

/*
 * PWM clock divisor:
 * in pwmfifo.c, we choose PLL_D (500MHz) as the clock source.
 */
#define PWM_CLOCK_DIV	25	/* 500MHz / 25 == 20MHz */

/*
 * PWM bit representation:
 *     ____                ________
 * 0: /    \________   1: /        \____
 *    <---->              <-------->
 *     T0H                  T1H
 *    <------------>
 *     T_CYCLE
 *
 * For WS2812B-based LED strips (e.g. <http://ssci.to/1400>):
 *   T_CYCLE = 1.25us +-600ns
 *   T0H     = 0.4 us +-150ns
 *   T1H     = 0.8 us +-150ns
 *   RESET code = Low >=50us
 */
#define T_CYCLE		25	/* 1.25us * 20MHz */
#define T0H		8	/* 0.4 us */
#define T1H		16	/* 0.8 us */
#define RST_BITS	40	/* RESET (50us) == 40 * 1.25us */

/*
 * Transmission order of R,G,B:
 * For WS2812B, the order is G,R,B.
 */
#define COLOR_ORDER	ORDER_GRB
#define ORDER_GRB	0
#define ORDER_RGB	1

/* The number of bits for each of R,G,B */
#define RGB_BITS  8
#define RGB_MAX   ((1 << RGB_BITS) - 1)


/* The number of LEDs in the strip */
static int nLed;

/* A buffer for keeping the color of each LED */
static unsigned int ledColor[MAX_N_LED];


/**
 * Setting up the hardware:
 * call this function at the beginning.
 * \param gpioPin  GPIO-number for the LED strip.
 * \param n        The number of LEDs in the LED strip.
 * \return  0 for success, -1 for failure.
 */
int ledSetup(int gpioPin, int n)
{
  if (setupGpio() == -1) {
    return -1;
  }
  pinModePwmFifo(gpioPin);
  pwmSetModeMS(gpioPin);	/* mark:space mode */
  pwmSetClock(PWM_CLOCK_DIV);
  pwmSetRange(gpioPin, T_CYCLE);

  if (n < 0) { return -1; }
  if (n > MAX_N_LED) { return -1; }
  nLed = n;
  return 0;
}

/**
 * Cleaning up: call this function at the end.
 */
void ledCleanup()
{
  ledClearAll();  /* Turn off all lights! */
  cleanupGpio();
}

#define PACK_COLOR(h,m,l)  (((h)<<(2*RGB_BITS))|((m)<<RGB_BITS)|(l))

/**
 * Set the color of one LED (not sent to the strip in this function)
 * \param led  The id of an LED (0〜)
 * \param r    The value of red   (0〜255)
 * \param g    The value of green (  "   )
 * \param b    The value of blue  (  "   )
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

/**
 * Send the color data to the LED strip!
 */
void ledSend()
{
  static unsigned char buf[MAX_N_LED * 3 * RGB_BITS + RST_BITS];
  int i, j;
  for (i = 0; i < nLed; i++) {
    int col = ledColor[i];
    int mask = (1 << (3 * RGB_BITS - 1));
    for (j = 0; j < 3 * RGB_BITS; j++) {
      buf[i * 3 * RGB_BITS + j] = ((col & mask) ? T1H : T0H);
      mask >>= 1;
    }
  }
  /* RESET code */
  for (i = 0; i < RST_BITS; i++) {
    buf[nLed * 3 * RGB_BITS + i] = 0;
  }
  pwmWriteBlock(buf, nLed * 3 * RGB_BITS + RST_BITS);
}

/**
 * Turn off all lights.
 */
void ledClearAll()
{
  int led;
  for (led = 0; led < nLed; led++) {
    ledSetColor(led, 0, 0, 0);
  }
  ledSend();
}

/**
 * Set the color of one LED in HSB (not sent to the strip in this function)
 * \param led  The id of an LED (0〜)
 * \param h    Hue        (0〜359)
 * \param s    Saturation (0〜255)
 * \param v    Brightness (  "   )
 */
void ledSetColorHSB(int led, int h, int s, int v)
{
  int hgroup;
  double f, ss;
  int p, q, t, r, g, b;

  /* constrain the values within ranges */
  if (h < 0)   { h = 0; }
  if (h > 359) { h = 359; }
  if (s < 0)   { s = 0; }
  if (v < 0)   { v = 0; }
  if (s > RGB_MAX) { s = RGB_MAX; }
  if (v > RGB_MAX) { v = RGB_MAX; }

  /* hue grouping (0..5) */
  hgroup = (h / 60) % 6;

  /* convert into r,g,b */
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

  /* use the results */
  ledSetColor(led, r, g, b);
}
