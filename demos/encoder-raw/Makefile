.PHONY: all clean

CFLAGS = -Wall -Wextra -Wconversion -Wdouble-promotion -g -O2 -I../..
LDFLAGS =

all: encoder-raw

encoder-raw: encoder-raw.o
	$(CC) -o $@ $^ $(LDFLAGS)

encoder-raw.o: encoder-raw.c ../../tflac.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f encoder-raw encoder-raw.o
