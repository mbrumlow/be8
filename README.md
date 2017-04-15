# be8 

This is a 8-bit computer emulator based off [Ben Eater's bread board computer](https://www.youtube.com/watch?v=HyznrdDSSGM). 

## Building 

```
make
```

This should build without much fuss on any modern Linux distro just by typing 'make'. 

## Running 

Normal 
```
./be8 [path to program]
```
Debug 
```
./be8 -d [path to program]
```

Included are two programs. Both programs were featured in Ben Eater's videos. 

- programs/add42.8 - Adds 14 and 28 to output 42 it can be seen in Ben's "[Programming my 8-bit breadboard computer](https://www.youtube.com/watch?v=9PPrrSyubG0)"

- programs/fib.8 - Displays the Fibonacci sequence as seen in Ben's "[Programming Fibonacci on a breadboard computer](https://www.youtube.com/watch?v=a73ZXDJtU48)"

You can also compile programs from be8 assembly to be8 machine code. This is done by using be8asm program. 

```
./be8asm [path to asm] -o [path to output program] 
```

Examples can be found in the programs directory. 
