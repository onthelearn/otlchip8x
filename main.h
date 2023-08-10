#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <cstdint>

#include "SDL.h"

using namespace std;

/*MACRO definitions**************************************************************************************************************************/

/*chip modes*/
#define COSMACVIP	0x1
#define CHIP48		0x2
#define SUPERCHIP	0x4
#define CHIPMODE  COSMACVIP //default chip mode

/*Registers*/
#define V0		V[0x0]
#define V1 		V[0x1]
#define V2 		V[0x2]
#define V3 		V[0x3]
#define V4 		V[0x4]
#define V5 		V[0x5]
#define V6 		V[0x6]
#define V7 		V[0x7]
#define V8 		V[0x8]
#define V9 		V[0x9]
#define VA		V[0xA]
#define VB		V[0xB]
#define VC		V[0xC]
#define VD		V[0xD]
#define VE		V[0xE]
#define VF		V[0xF]

/*instruction extractions*/
#define ITYPE	(instruction>>12)				//instruction type(Most significant Byte)
#define X		((instruction & 0x0F00) >> 8)	//X Register name
#define VX		V[X]							//X Register
#define VY		V[(instruction & 0x00F0) >> 4]	//Y Register
#define N		(instruction & 0x000F)			//Immediate
#define NN		(instruction & 0x00FF)			//Immediate
#define NNN		(instruction & 0x0FFF)			//Address

/*font address translation*/
#define OFFSET_FONT			0x0050				//Address where font data begins
#define BYTES_PER_FONT		5					//Each font sprite needs 5 byte
#define font(x)				(OFFSET_FONT + ((x & 0x000F)*BYTES_PER_FONT)) //Get font x address

/*load ROM*/
#define OFFSET_ROM			0x0200	// Address where ROM data begins

/*SDL rendering*/
#define SCALE_FACTOR			10	//scale factor for 64x32 pixels (Square pixel length)
#define CHIP8_DISPLAY_WIDTH		64	//Chip8 screen width
#define CHIP8_DISPLAY_HEIGHT	32	//Chip8 screen height
#define WINDOW_WIDTH			(CHIP8_DISPLAY_WIDTH*scaleFactor)	//Display window width
#define WINDOW_HEIGHT			(CHIP8_DISPLAY_HEIGHT*scaleFactor)	//Display window height

/*SDL audio, handle audio (sine wave)*/
#define SINE_FREQUENCY			480			//tone frequency
#define SAMPLING_FREQUENCY		48000		//wave data samples per seconds
#define N_SAMPLES				(SAMPLING_FREQUENCY / SINE_FREQUENCY) //data samples per cycle
#define WAVE_DATA_TYPE			Sint8		//wave data sample size (8 bit here for mono channel single byte data)
#define WAVE_LENGTH			 (N_SAMPLES)	// #values for a single wavelength of wave


/*handle timing*/
#define ENABLE_DELAY 1			//enable/disable CPU frequency limit, 0 if no limit
#define FREQUENCY_TO_MILLIS(x) (1000.0/x)	//convert frequency to milliseconds
#define FREQUENCY_TIMER 60 //Hz	//timers #(down)counts per seconds
#define FRAME_RATE 60//Hz		//frames per seconds
#define FREQUENCY_CPU 700//Hz	//CPU frequency limit

/**Type Definitions********************************************************************************************************************/
typedef uint8_t Reg8;	//8 bit reg
typedef uint16_t Reg16; //16 bit reg

/**Global Variables*********************************************************************************************************************/

/*Chip components*/
extern uint8_t ram[4096];		//ram 4KB
extern bool vram[64][32];		//display pixel data, 64x32 pixels
extern uint8_t stackPointer;	//max 255 (0-255)
extern uint16_t stack[256];		//stored addresses 16bit
extern Reg16 PC, I; 			//16-bit Program Counter and Index Register
extern Reg8 V[16];				//Register file with 16 general purpose registers 8 bit
extern Reg8 timerDelay;			//Down counter, 8 bit, 60 Hz
extern Reg8 timerSound;			//Down counter, 8 bit, 60 Hz, beep when non zero

/*ROM*/
extern const char* romFilename; //store ROM filename

/*SDL Rendering*/
extern SDL_Window* window;		//SDL window
extern SDL_Renderer* renderer;	//SDL renderer

/*SDL Audio*/
extern SDL_AudioSpec spec;		//tone audio specification
extern SDL_AudioDeviceID dev;	//audio device
extern WAVE_DATA_TYPE wave[WAVE_LENGTH];	//one wavelength data of tone

/*For key presses, FX0A statemachine two bits, and event (close)*/
extern bool keyIsPressed;		//key was pressed or not or released
extern bool waitingKeyPress;	//waiting for a key press
extern uint8_t pressedKeyHex;	//last pressed key
extern SDL_Event e;				//SDL event queue, tells wether keyboard key pressed or close(X) button clicked
extern bool quit;				//tell wether to quit app

/*timing*/
extern uint32_t lastTimerUpdate;	//for down counter registers
extern uint32_t lastFrameUpdate;	//for rendering
extern uint32_t lastCPUExecute;//for cpu frequency 

/*offline configuration */
extern int scaleFactor;
extern int displayWidth;
extern int displayHeight;
extern int enableDelay;
extern int frequencyTimer;
extern int frameRate;
extern int frequencyCPU;

/*Functions***************************************************************************************************/
void init();			//clear chip,  load config, load font, load rom
void chip8Clear();		//clear all register, ram, vram
void loadConfig();		//load or creae config file with configurable options
void loadFont();		//pre load font
void loadProgram();		//read rom file

void push(uint16_t address);	//stack push
uint16_t pop();					//stack pop

uint16_t fetch();				//fetch next rom instruction
void decodeandexecute(uint16_t instruction);	//decode and execute the instruction

void run();			//infinite loop where emulator runs, also polls events
void emulate();		//perform fetch, decode, execute, display, audio with timing considereation

void initDisplay();	//initialize SDL video 
void clearDisplay();	//clear vram
bool setPixel(uint8_t x, uint8_t y, bool bit);	//set pixel value by XORing bit with current pixel
bool draw(uint8_t x, uint8_t y, uint8_t num);	//set #num pixels from (x,y)
void renderToSDLWindow();	//update display window
void renderToConsole();		//for debug, display vram with characters on the console window	

int initAudio();			//initialize SDL audio, start audio device
WAVE_DATA_TYPE getSineWaveSample();	//get circular wave data (one at a time), if wave data ends, next data starts from beginning
void audioCallback(void* userdata, Uint8* stream, int len);	//callback functions when audio stream data ends (audio buffer empty). Provides spec.samples amount of data

void handleKeyDown();	//signal key press or quit action

