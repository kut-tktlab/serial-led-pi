int setupGpio();
int cleanupGpio();

void pinModePwm(int pin);
void pwmSetModeBalanced();
void pwmSetModeMS();
void pwmSetClock(unsigned int divider);
void pwmSetRange(unsigned int range);
void pwmWriteBlock(const unsigned char *array, int n);
void pwmWaitFifoEmpty();
