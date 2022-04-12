# simple makefile

all: mas

mas: encode.c encode.h parser.c parser.h symtab.c symtab.h writer.c writer.h main.c
	gcc -O2 encode.c parser.c symtab.c writer.c main.c -o mas

clean:
	-rm mas a.mxe
