CFLAGS = -W -Wall
LDFLAGS =

.PHONY: all
all: serialled.so

serialled.so: serialled.o pwmfifo.o mailbox.o
	$(CC) $(LDFLAGS) $+ -shared -o $@

.PHONY: clean
clean:
	$(RM) *.o *.so a.out *.pyc
