.PHONY: all clean

CFLAGS = -Wall -Wextra -g

all: weird-wav-maker

weird-wav-maker: weird-wav-maker.o
	$(CC) -o $@ $^ $(LDFLAGS)

weird-wav-maker.o: weird-wav-maker.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f weird-wav-maker.o weird-wav-maker
