CFLAGS = -W -Wall
LDFLAGS = $(LIBS)

ledtape: test.o serialled.o pwmfifo.o
	$(CC) $(LDFLAGS) $+ -o $@

.PHONY: clean
clean:
	$(RM) *.o a.out *.pyc ledtape
