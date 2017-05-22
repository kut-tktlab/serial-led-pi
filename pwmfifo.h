int setupGpio();

void pinModePwm(int pin);
void pwmSetModeBalanced();
void pwmSetModeMS();
void pwmSetClock(unsigned int divider);
void pwmSetRange(unsigned int range);
void pwmWriteBlock(const unsigned int *array, int n);
void pwmWaitFifoEmpty();

void delayMicroseconds(unsigned int usec);
