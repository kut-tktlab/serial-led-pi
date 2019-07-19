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
