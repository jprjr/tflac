.PHONY: all clean

CFLAGS = -Wall -Wextra -g -O2 -I../..
LDFLAGS =

all: encoder

encoder: encoder.o
	$(CC) -o $@ $^ $(LDFLAGS)

encoder.o: encoder.c ../../tflac.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f encoder encoder.o
