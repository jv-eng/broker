all: server client prueba

server:
	cd broker; make

client:
	cd lib_cl; make

prueba:
	cd test; make 

clean:
	cd broker; make clean
	cd lib_cl; make clean
	cd test; make clean