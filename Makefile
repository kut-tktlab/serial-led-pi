CFLAGS = -W -Wall
LDFLAGS = $(LIBS)

ledtape: ledtape.o pwmfifo.o
	$(CC) $(LDFLAGS) $+ -o $@

.PHONY: clean
clean:
	$(RM) *.o a.out *.pyc ledtape
