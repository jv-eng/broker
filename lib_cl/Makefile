CFLAGS=-Wall -g -fPIC

libzerocopyMQ.so: lib_cl.o comun.o
	$(CC) $(CFLAGS) -shared -o $@ $^

libzerocopyMQ.o: comun.h  lib_cl.h

clean:
	rm -f *.o *.so
