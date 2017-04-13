
all: be8asm be8

be8: be8.c
	cc     be8.c   -o be8

be8asm: be8asm.c 
	cc     be8asm.c   -o be8asm

clean: 
	rm -f be8
	rm -f be8asm
