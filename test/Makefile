CFLAGS=-Wall -g

all: lib_cl test

lib_cl:
	$(MAKE) -C ../lib_cl

test: test.o lib_cl.so
	$(CC) -o $@ $< ./lib_cl.so

test.o: lib_cl.h

clean:
	rm -f *.o test


