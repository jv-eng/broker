all:
	cd broker/util; make
	cd broker; make
	cd libedsu; make
	cd test; make

clean:
	cd broker/util; make clean
	cd broker; make clean
	cd libedsu; make clean
	cd test; make clean