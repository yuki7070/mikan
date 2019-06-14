CFLAGS=-std=c11 -Wall -g -static

mikan: mikan.c

test: mikan
	bash ./test.sh

clean:
	rm -f mikan *.o *~ tmp*
