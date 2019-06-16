CFLAGS=-std=c11 -Wall -g -static

mikan: mikan.c

test: mikan
	bash ./test.sh
	./mikan -test

clean:
	rm -f mikan *.o *~ tmp*
