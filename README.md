# otlchip8x
Cross platform chip8 emulator/interpreter

## Install
Look at the releases or download source codes from here:

For windows x64:

1) Extract zip file in a folder or download .exe file, download SDL.dll from their SDL website
2) drag and drop a chip8 rom
3) Change created config file as needed

For linux:

1) Extract source code (main.cpp main.h in a folder)
2) Libraries needed to compile: 
```
sudo apt-get install libsdl2-dev g++
```
3) Compile:
```
g++ -o otlchip8x main.cpp `sdl2-config --cflags --libs`
```
4) Provide rom as argument or drag and drop
5) Change configuration file as needed

## Roms:

Huge collection of roms hosted by [kripod](https://github.com/kripod/chip8-roms)
Some tests by [Timendus](https://github.com/Timendus/chip8-test-suite)
## Keymap
QWERTY(or same key position as QWERTY)  => Hex Value recognized by Chip8
```
1 2 3 4 => 1 2 3 C
Q W E R => 4 5 6 D
A S D F => 7 8 9 E
Z X C V => A 0 B F
```

## Extra Info
1) Space Invades : change CHIPMODE to 2 or 4 in configuration file.
