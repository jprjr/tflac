.PHONY: all clean

CFLAGS = -pthread -Wall -Wextra -Wconversion -Wdouble-promotion -g -O2 -I../..
LDFLAGS = -pthread

all: encoder-raw-mt

encoder-raw-mt: encoder-raw-mt.o
	$(CC) -o $@ $^ $(LDFLAGS)

encoder-raw-mt.o: encoder-raw-mt.c ../../tflac.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f encoder-raw-mt encoder-raw-mt.o
