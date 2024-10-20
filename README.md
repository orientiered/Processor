# Processor
**SPU** - software processing unit.

Runs programs written in assembler-like language.

+ built-in stack, registers and RAM.

+ jumps and function calls

+ some graphics using `draw` and `drawr` commands

**WARNING**: processor uses some system-dependent libraries, like `unistd.h` and `sys/time.h`, so it works **ONLY** on LINUX.

Compile both assembler and spu - `make`

Compile just the program you want with `make asm` or `make spu`

Delete object files with `make clean`

## How to use:

### Processor
```
./spu.out -i <filename>
```
You can activate logger using `-d` flag

### Assembler
```
./asm.out -i <inputFile> -o <outputFile>
```
`-o` flag is optional, without it output file will have the same name as input, but with extension `.lol`

You can activate logger using `-d` flag:
1. `-d 1` prints some debug info
2. `-d 2` logs every step of assembling

## Examples

You can try programs from `asmProgs/` folder, or ran badApple program from `bad-apple-converter/badApple.asm`
