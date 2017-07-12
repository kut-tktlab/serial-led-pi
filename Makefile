CFLAGS = -W -Wall
LDFLAGS =
OBJS = serialled.o pwmfifo.o mailbox.o

.PHONY: all
all: serialled.so

serialled.so: $(OBJS)
	$(CC) $(LDFLAGS) $+ -shared -o $@

.PHONY: bky-led
bky-led: bky-led.cc $(OBJS:.o=.c) binding.gyp
	node-gyp configure build

.PHONY: clean
clean:
	$(RM) *.o *.so a.out *.pyc
