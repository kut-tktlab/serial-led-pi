/*
 * @param x  degree
 * @return  -255 to 255
 */
int sindeg(int x);

/*
 * @param x  degree
 * @return  -255 to 255
 */
#define cosdeg(x)  (sindeg((x) + 90))


/*
 * pseudo random generator
 */
#define RAND_MAX  32767
int rand(void);
void srand(unsigned int);
