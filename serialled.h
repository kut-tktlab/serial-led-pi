/* 最大LED数 */
#define MAX_N_LED  100

/* セットアップするよ */
int ledSetup(int gpioPin, int n);

/* 後片付け */
void ledCleanup(void);

/* 1素子の色を設定 (まだ送信しない) */
void ledSetColor(int led, int r, int g, int b);

/* 1素子の色をHSBで設定 (まだ送信しない) */
void ledSetColorHSB(int led, int h, int s, int v);

/* 色情報を送信! */
void ledSend(void);

/* 全部消しましょう */
void ledClearAll(void);
