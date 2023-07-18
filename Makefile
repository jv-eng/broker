all:
	cd editor; make
	cd intermediario; make
	cd subscriptor; make

clean:
	cd editor; make clean
	cd intermediario; make clean
	cd subscriptor; make clean