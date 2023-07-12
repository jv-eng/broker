CFLAGS=-Wall

all: intermediario

intermediario.o: dict.h cola.h comun.h
comun.o: comun.h

intermediario: intermediario.o comun.o dict.o cola.o
	$(CC) -o $@ intermediario.o comun.o dict.o cola.o -lpthread

clean:
	rm -f *.o intermediario
