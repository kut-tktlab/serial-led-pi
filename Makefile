CFLAGS = -W -Wall
LDFLAGS = $(LIBS)

.PHONY: all
all: ledtape rainbow

ledtape: test.o serialled.o pwmfifo.o
	$(CC) $(LDFLAGS) $+ -o $@

rainbow: rainbow.o serialled.o pwmfifo.o
	$(CC) $(LDFLAGS) $+ -o $@

.PHONY: clean
clean:
	$(RM) *.o a.out *.pyc ledtape rainbow
