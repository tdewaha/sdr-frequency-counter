CFLAGS?=-O2 -g -Wall -W $(shell pkg-config --cflags librtlsdr)
LDLIBS+=$(shell pkg-config --libs librtlsdr) -lpthread -lm
CC?=gcc
PROGNAME=fcounter

all: fcounter

%.o: %.c
	$(CC) $(CFLAGS) -c $<

fcounter: fcounter.o kiss_fft.o
	$(CC) -g -o fcounter fcounter.o kiss_fft.o $(LDFLAGS) $(LDLIBS)
clean:
	rm -f *.o fcounter
