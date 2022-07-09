
all: servidor.o cliente.o split.o files.o
	gcc -o servidor servidor.o split.o files.o -pthread
	gcc -o cliente cliente.o split.o files.o

servidor.o: servidor.c 
	gcc -c -o servidor.o servidor.c

cliente.o: cliente.c 
	gcc -c -o cliente.o cliente.c

clean:
	rm -f servidor cliente *.o 
