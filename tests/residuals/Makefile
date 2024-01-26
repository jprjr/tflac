.PHONY: all clean test time

CFLAGS = -I../.. -Wall -Wextra -g -O3

all: test-64bit test-32bit time-64bit time-32bit

test: test-64bit test-32bit
	echo "Native 64 bit integers"
	./test-64bit
	echo "Emulated 64 bit integers"
	./test-32bit

time: time-64bit time-32bit
	echo "Native 64 bit integers"
	./time-64bit
	echo "Emulated 64 bit integers"
	./time-32bit

test-64bit: test.c
	$(CC) $(CFLAGS) -o $@ $^

test-32bit: test.c
	$(CC) $(CFLAGS) -DTFLAC_32BIT_ONLY -o $@ $^

time-64bit: time.c
	$(CC) $(CFLAGS) -o $@ $^

time-32bit: time.c
	$(CC) $(CFLAGS) -DTFLAC_32BIT_ONLY -o $@ $^

clean:
	rm -f test-64bit test-32bit
	rm -f test-64bit.exe test-32bit.exe
	rm -f time-64bit time-32bit
	rm -f time-64bit.exe time-32bit.exe
