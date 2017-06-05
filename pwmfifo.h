int setupGpio();
int cleanupGpio();

int pinModePwm(int pin);
int pinModePwmFifo(int pin);
void pwmSetModeBalanced(int pin);
void pwmSetModeMS(int pin);
void pwmSetClock(unsigned int divider);
void pwmSetRange(int pin, unsigned int range);
void pwmWrite(int pin, unsigned int data);
void pwmWriteBlock(const unsigned char *array, int n);
void pwmWaitFifoEmpty();
