CFLAGS = -W -Wall
LDFLAGS =
OBJS = serialled.o pwmfifo.o mailbox.o

.PHONY: all
all: serialled.so

serialled.so: $(OBJS)
	$(CC) $(LDFLAGS) $+ -shared -o $@

bkyreceiver: bkyreceiver.o $(OBJS)
	$(CC) $(LDFLAGS) $+ -o $@

.PHONY: clean
clean:
	$(RM) *.o *.so a.out *.pyc
