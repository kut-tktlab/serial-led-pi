/* 最大LED数 */
#define MAX_N_LED  100

/* セットアップするよ */
int ledSetup(int gpioPin, int n);

/* 1素子の色を設定 (まだ送信しない) */
void ledSetColor(int led, int r, int g, int b);

/* 色情報を送信! */
void ledSend();

/* 全部消しましょう */
void ledClearAll();
