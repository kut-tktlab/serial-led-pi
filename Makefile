CFLAGS = -W -Wall
LDFLAGS =

.PHONY: all
all: serialled.so bkyreceiver

serialled.so: serialled.o pwmfifo.o mailbox.o
	$(CC) $(LDFLAGS) $+ -shared -o $@

bkyreceiver: bkyreceiver.o serialled.o pwmfifo.o mailbox.o
	$(CC) $(LDFLAGS) $+ -o $@

.PHONY: clean
clean:
	$(RM) *.o *.so a.out *.pyc
