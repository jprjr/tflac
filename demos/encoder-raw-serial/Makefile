.PHONY: all clean

CFLAGS = -Wall -Wextra -Wconversion -Wdouble-promotion -g -O2 -I../..
LDFLAGS =

all: encoder-raw-serial

encoder-raw-serial: encoder-raw-serial.o
	$(CC) -o $@ $^ $(LDFLAGS)

encoder-raw-serial.o: encoder-raw-serial.c ../../tflac.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f encoder-raw-serial encoder-raw-serial.o
