CFLAGS=-std=c11 -Wall -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

mikan: $(OBJS)
		$(CC) -o mikan $(OBJS) $(LDFLAGS)

$(OBJS): mikan.h

test: mikan
	bash ./test.sh
	./mikan -test

clean:
	rm -f mikan *.o *~
