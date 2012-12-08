all:
	gcc -c error.c -Wall -ansi -pedantic
	gcc -c safe.c -Wall -ansi -pedantic
	gcc -o os-detect os-detect.c -Wall -ansi -pedantic
	gcc -lrt -lpthread -pthread -c safeexec.c `./os-detect` -Wall -ansi -pedantic
	gcc -lrt -lpthread -pthread -o safeexec error.o safeexec.o safe.o -Wall -ansi -pedantic

clean:
	rm -rf *.o safeexec os-detect
