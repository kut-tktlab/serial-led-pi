/*
 * Make an LED-strip glitter like raindrops.
 * (c) yoshiaki takata, 2019
 */

#include "serialled.h"
#include "pwmfifo.h"	/* delayMicroseconds */
#include "sin.h"

/* GPIO番号 */
#define LED_GPIO  18

/* LEDの個数 */
#define N_STRIP    4
#define STRIP_LEN  10
#define N_LED      (N_STRIP * STRIP_LEN)

/* frame per second */
#define FPS  60

/*
 * Frame buffer
 */

typedef struct {
  int h, s, b;
} Cell;

Cell framebuf[N_STRIP][STRIP_LEN];

int hue = 180;
int sat = 200;

void clearScreen()
{
  int i, j;
  for (i = 0; i < N_STRIP; i++) {
    for (j = 0; j < STRIP_LEN; j++) {
      framebuf[i][j].h = hue;
      framebuf[i][j].s = sat;
      framebuf[i][j].b = 0;
    }
  }
}

void sendScreen()
{
  int i, j;
  for (i = 0; i < N_STRIP; i++) {
    for (j = 0; j < STRIP_LEN; j++) {
      int led = i * STRIP_LEN + j;
      int h = framebuf[i][j].h;
      int s = framebuf[i][j].s;
      int b = framebuf[i][j].b;

      /* 色を設定 (まだ送信しない) Set the color (but not transmit yet). */
      ledSetColorHSB(led, h, s, b);
    }
  }

  /* 送信! Transmit the color data! */
  ledSend();
}

/*
 * Stars
 */

#define N_STAR  (2 * N_STRIP)
#define min(a, b)  ((a) < (b) ? (a) : (b))

typedef struct {
  int alive;  /* 0 or 1 */
  int strip;  /* 0..<N_STRIP */
  int head;   /* position */
  int hue;
} Star;

Star star[N_STAR];
int starDecay = 4;

void initRandomStar(int i)
{
  star[i].strip = i % N_STRIP;
  star[i].head  = -FPS * rand() * 4 / (RAND_MAX + 1) - FPS;

  int j = (i + N_STRIP) % N_STAR;
  if (star[j].alive && star[j].head < 30) {
    star[i].head = min(star[i].head, star[j].head - FPS);
  }
  star[i].hue   = rand() * 256 / (RAND_MAX + 1);
  star[i].alive = 1;
}

void initLaunchStar(int i)
{
  if (i < N_STRIP) {
    star[i].head = -FPS - i * STRIP_LEN * 3 / 2;
  } else if (i < 2 * N_STRIP) {
    star[i].head = -FPS + (-3 * N_STRIP + i) * STRIP_LEN * 3 / 2;
  }
  star[i].hue   = rand() * 256 / (RAND_MAX + 1);
  star[i].alive = 1;
}

void resetLaunchStar(int i)
{
  star[i].head  = -FPS * 5 / 2;
  star[i].hue   = rand() * 256 / (RAND_MAX + 1);
  star[i].alive = 1;
}

void moveStar(int i)
{
  star[i].head += 1;
  if (star[i].head > 60) {
    star[i].alive = 0;
  }
}

void drawStar(int i)
{
  int j;
  for (j = 1; j >= -60; j--) {
    int x = star[i].head + j;
    int b;
    if (x < 0 || STRIP_LEN <= x) { continue; }
    if (j > 0) {
      b = 127;
    } else if (j == 0) {
      b = 255;
    } else {
      b = 200 - (-j - 1) * starDecay;
      b = b < 0 ? 0 : b > 255 ? 255 : b;
    }
    if (b >= framebuf[star[i].strip][x].b) {
      framebuf[star[i].strip][x].h = star[i].hue;
    }
    b += framebuf[star[i].strip][x].b;
    b  = b > 255 ? 255 : b;
    framebuf[star[i].strip][x].b = b;
  }
}

#define MODE_RANDOM   0
#define MODE_WAITING1 1
#define MODE_WAITING2 2
#define MODE_LAUNCH   3

int main()
{
  int t;
  int i;
  int mode, mt;

  /* セットアップするよ */
  if (ledSetup(LED_GPIO, N_LED) == -1) {
    return -1;
  }

  /* Initialize the stars */
  for (i = 0; i < N_STAR; i++) {
    initRandomStar(i);
  }

  /* 6時間点灯するよ Glitter for 6 hours. */
  mode = MODE_RANDOM;
  mt = 0;
  for (t = 0; t < 6 * 3600 * FPS; t++, mt++) {
    clearScreen();  /* clear the frame buffer */

    /* Move each star */
    int nalive = 0;
    for (i = 0; i < N_STAR; i++) {
      if (star[i].alive) {
        moveStar(i);
        drawStar(i);
        nalive++;
      }
    }

    /* Reset inactive stars */
    for (i = 0; i < N_STAR; i++) {
      if (star[i].alive) { continue; }
      int r;
      switch (mode) {
      case MODE_RANDOM:
        r = rand() * 100 / (RAND_MAX + 1);
        if (r < 2) {
          initRandomStar(i);
        }
        break;
      case MODE_LAUNCH:
        star[i].strip = i % N_STRIP;
        if (mt <= 1) {
          initLaunchStar(i);
        } else {
          resetLaunchStar(i);
        }
        break;
      }
    }

    /* Mode shift */
    if (mode == MODE_RANDOM && mt > 60 * FPS) {
      mode = MODE_WAITING1;
      mt = 0;
    }
    if (mode == MODE_WAITING1 && nalive == 0) {
      mode = MODE_LAUNCH;
      mt = 0;
      starDecay = 6;
    }
    if (mode == MODE_LAUNCH && mt > 8 * FPS) {
      mode = MODE_WAITING2;
      mt = 0;
    }
    if (mode == MODE_WAITING2 && mt > 8 * FPS && nalive == 0) {
      mode = MODE_RANDOM;
      mt = 0;
      starDecay = 4;
    }

    sendScreen();  /* transmit the contents of the frame buffer */

    /* 次のフレームまで待ってください Wait until the next frame. */
    delayMicroseconds(1000 * 1000 / FPS);
  }

  /* 全部消しましょう */
  ledClearAll();

  return 0;
}
