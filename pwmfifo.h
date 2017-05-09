int setupGpio();

void pinModePwm(int pin);
void pwmSetModeBalanced();
void pwmSetModeMS();
void pwmSetClock(unsigned int divider);
void pwmSetRange(unsigned int range);
void pwmWriteFifo(unsigned int byte);
void pwmWaitFifoEmpty();

void delayMicroseconds(unsigned int usec);
