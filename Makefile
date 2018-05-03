cc=gcc
CFLAGS+=-m64 -g

ASM=nasm
AFLAGS=-f elf64 -g
LIBS=`sdl2-config --cflags --libs`

all: scaler 

main.o: main.c
	$(CC) $(CFLAGS) -c main.c $(LIBS) 
resize.o: resize.asm
	$(ASM) $(AFLAGS) resize.asm
scaler: main.o resize.o
	$(CC) $(CFLAGS) main.o resize.o -o scaler  $(LIBS) 
clean:
	rm *.o
	rm scaler 
