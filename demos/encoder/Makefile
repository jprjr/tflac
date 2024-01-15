.PHONY: all clean

CFLAGS = -Wall -Wextra -Wconversion -Wdouble-promotion -g -O2 -I../..
LDFLAGS =

all: encoder

encoder: encoder.o wavdecoder.o
	$(CC) -o $@ $^ $(LDFLAGS)

encoder.o: encoder.c ../../tflac.h
	$(CC) $(CFLAGS) -c -o $@ $<

wavdecoder.o: wavdecoder.c wavdecoder.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f encoder encoder.o wavdecoder.o
