/*
 * シリアルLEDテープを点灯させるよ
 */

#include <stdio.h>
#include <stdlib.h>

#include "pwmfifo.h"

/* LEDの個数 */
#define N_LED  10

/* LEDテープのGPIO番号 */
#define LED_GPIO  18

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

int main()
{
  int i, j;

  /* セットアップするよ */
  if (setupGpio() == -1) {
    fprintf(stderr, "cannot setup gpio.\n");
    return -1;
  }

  pinModePwm(LED_GPIO);
  pwmSetClock(PWM_CLOCK_DIV);
  pwmSetModeMS();	/* mark:spaceモードだよ */
  pwmSetRange(PWM_RANGE);

  /* 5回点灯するよ */
  for (i = 0; i <= 5; i++) {
    int led, rgb, bit, myColor;
    int turnOn = (i < 5);	/* 今は点けるとき? */
    for (led = 0; led < N_LED; led++) {
      for (rgb = 0; rgb < 3; rgb++) {
        /* 各LEDがRGBの1色ずつ選ぶよ */
        myColor = ((led + i) % 3 == rgb);
        for (bit = 0; bit < 8; bit++) {
          pwmWriteFifo((turnOn && myColor) ? ONE : ZERO);
        }
      }
    }
    /* RET信号 */
    for (j = 0; j < SPACE; j++) {
      pwmWriteFifo(0);
    }

    delayMicroseconds(1000 * 1000);	/* 1秒待ってください */
  }

  /* Wait until the fifo becomes empty */
  pwmWaitFifoEmpty();

  return 0;
}
