/*
 * sin without floating-point arithmetics
 */

#include "sin.h"

#define PI   205887  /* M_PI * (2 ** 16) */
#define ADJ  (1 << 16)

static long long longdiv(long long t, unsigned short x) {
  unsigned int h[4];
  int sign = 1;
  if (t < 0) {
    sign = -sign;
    t = -t;
  }
  h[0] = ( t        & 0xffff);
  h[1] = ((t >> 16) & 0xffff);
  h[2] = ((t >> 32) & 0xffff);
  h[3] = ((t >> 48) & 0xffff);
  h[2] += (h[3] % x) << 16; h[3] /= x;
  h[1] += (h[2] % x) << 16; h[2] /= x;
  h[0] += (h[1] % x) << 16; h[1] /= x;
  h[0] /= x;
  t  = h[3]; t <<= 16;
  t += h[2]; t <<= 16;
  t += h[1]; t <<= 16;
  t += h[0];
  return sign * t;
}

/*
 * @param x  degree
 * @return  -255 to 255
 */
int sindeg(int x)
{
  int res;
  long long term;
  int i;
  int sign = 1;
  if (x < 0) {
    x = -x;
    sign = -1;
  }
  while (x > 360) {
    x -= 360;
  }
  if (x > 180) {
    x = 360 - x;
    sign = -sign;
  }
  if (x > 90) {
    x = 180 - x;
  }
  x = x * PI / 180;
  res = 0;
  term = x;
  for (i = 1; res + term != res && i < 20; i += 2) {
    res += term;
    term = -term * x / ADJ * x / ADJ;
    term = longdiv(term, i + 1);
    term = longdiv(term, i + 2);
  }
  res /= ADJ / 256;
  if (res >= 256) { res = 255; }
  return sign * res;
}


/*
 * simple pseudo random generator
 * (a sample implementation in POSIX IEEE 1003.1)
 */
static unsigned long next = 1;
int rand(void) { /* RAND_MAX assumed to be 32767. */
  next = next * 1103515245 + 12345;
  return (unsigned)(next / 65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

#if STANDALONE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main()
{
  long long t = 100000;
  printf("%lld\n", longdiv(t * t * t, 101));
  int dc = 0, ds = 0;
  int xx;
  for (xx = 0; xx <= 360; xx += 1) {
    float x = xx * M_PI / 180;
    int c = cosdeg(xx);
    int s = sindeg(xx);
    int mc = (int)(cos(x) * 255);
    int ms = (int)(sin(x) * 255);
    //printf("cos(%f %d)=%d, %d, %d\n", x, xx, mc, c, mc - c);
    printf("sin(%f %d)=%d, %d, %d\n", x, xx, ms, s, ms - s);
    dc += abs(mc - c);
    ds += abs(ms - s);
  }
  printf("dc=%d, ds=%d\n", dc, ds);
  for (xx = 0; xx < 10; xx++) {
    printf("%d\n", rand());
  }
  return 0;
}
#endif
