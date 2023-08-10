#include "main.h"
#include <iostream>


/**	offline configuration **/
uint8_t mode;
int scaleFactor;
int displayWidth;
int displayHeight;
int enableDelay;
int frequencyTimer;
int frameRate;
int frequencyCPU;


/*Chip components*/
uint8_t ram[4096];		//ram 4KB
bool vram[64][32];		//display pixel data, 64x32 pixels
uint8_t stackPointer;	//max 255 (0-255)
uint16_t stack[256];	//stored addresses 16bit
Reg16 PC, I; 			//16-bit Program Counter and Index Register
Reg8 V[16];				//Register file with 16 general purpose registers 8 bit
Reg8 timerDelay;		//Down counter, 8 bit, 60 Hz
Reg8 timerSound;		//Down counter, 8 bit, 60 Hz, beep when non zero


/*timing*/
uint32_t lastFrameUpdate = 0;
uint32_t lastTimerUpdate = 0;
uint32_t lastCPUExecute = 0;


/*SDL************************************/
/*display*/
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

/*audio*/
SDL_AudioSpec spec;		//beep tone spec
SDL_AudioDeviceID dev;	//audio device
WAVE_DATA_TYPE wave[WAVE_LENGTH];	//beep data one wavelength(cycle)

/*keypress*/
bool keyIsPressed = false;
bool waitingKeyPress = true;
uint8_t pressedKeyHex = 0x00;

/*events (keypress or close)*/
SDL_Event e;
bool quit = false;
/***************************************/

const char* romFilename;

int main(int argc, char* argv[])
{
	romFilename = argv[1];

	init();
	//prepare screen
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		printf("SDL failed to initialize : %s\n", SDL_GetError());
		return -1;
	}

	initDisplay();

	initAudio();

	//loop
	run();

	//close
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void chip8Clear()
{
	//set all to 0
	mode = 0;

	for (int i = 0; i < 4096; ++i)
		ram[i] = 0;

	for (int i = 0; i < 64; ++i)
		for (int j = 0; j < 32; ++j)
			vram[i][j] = 0;

	stackPointer = 0;

	for (int i = 0; i < 256; ++i)
		stack[i] = 0;

	PC = 0;

	I = 0;

	for (int i = 0; i < 16; ++i)
		V[i] = 0;

	timerDelay = 0;

	timerSound = 0;
}

void loadFont()
{
	uint8_t  font[] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	for (int i = 0; i < 80; ++i) ram[OFFSET_FONT + i] = font[i];
}


void loadConfig()
{
	//set default values
	scaleFactor = SCALE_FACTOR;
	displayWidth = CHIP8_DISPLAY_WIDTH;
	displayHeight = CHIP8_DISPLAY_HEIGHT;
	enableDelay = ENABLE_DELAY;
	frequencyTimer = FREQUENCY_TIMER;
	frameRate = FRAME_RATE;
	frequencyCPU = FREQUENCY_CPU;
	mode = CHIPMODE;

	FILE* config = fopen("config.txt", "r");
	if (config == NULL)//create if does not exist
	{
		config = fopen("config.txt", "r");
		if (config == NULL) { quit = 1; return; }

		//write configuration structure to text
		fprintf(config, "%s %d\n", "ENABLE_DELAY", enableDelay);
		fprintf(config, "%s %d\n", "FREQUENCY_CPU", frequencyCPU);
		fprintf(config, "%s %d\n", "SCALE_FACTOR", scaleFactor);
		fprintf(config, "%s %d\n", "FRAME_RATE", frameRate);
		fprintf(config, "%s %hhu\n", "CHIP_MODE", mode);
		fclose(config);

		config = fopen("config.txt", "r");
	}

	fscanf_s(config, "%*s %d", &enableDelay);
	fscanf_s(config, "%*s %d", &frequencyCPU);
	fscanf_s(config, "%*s %d", &scaleFactor);
	fscanf_s(config, "%*s %d", &frameRate);
	fscanf_s(config, "%*s %hhu", &mode);

	fclose(config);

	printf("%s %d\n", "ENABLE_DELAY", enableDelay);
	printf("%s %d\n", "FREQUENCY_CPU", frequencyCPU);
	printf("%s %d\n", "SCALE_FACTOR", scaleFactor);
	printf("%s %d\n", "FRAME_RATE", frameRate);
	printf("%s %d\n", "CHIP_MODE", mode);
}


void loadProgram()
{
	//mode = COSMACVIP; //mode = SUPERCHIP;
	//loadProgram("test_opcode.ch8");
	//loadProgram("chip8-test-rom-with-audio.ch8");
	//loadProgram("IBM Logo.ch8");
	//loadProgram("chip8testsuite/2-ibm-logo.ch8");
	//loadProgram("chip8testsuite/3-corax+.ch8");
	//loadProgram("chip8testsuite/4-flags.ch8");
	//loadProgram("chip8testsuite/5-quirks.ch8"); //mode = SUPERCHIP;
	//loadProgram("bc_test.ch8"); mode = SUPERCHIP;
	//loadProgram("SCTEST"); //mode = SUPERCHIP;
	//loadProgram("chip8testsuite/6-keypad.ch8");
	//loadProgram("roms/flightrunner.ch8");
	//loadProgram("roms/delay_timer_test.ch8");
	//loadProgram("roms/Space Invaders [David Winter].ch8");mode = SUPERCHIP;
	//loadProgram("roms/Space Invaders [David Winter] (alt).ch8");mode = SUPERCHIP;
	//loadProgram("roms/chipquarium.ch8");
	//loadProgram("roms/cavern.ch8"); //mode = SUPERCHIP;
	//loadProgram("kripod/programs/SQRT Test [Sergey Naydenov, 2010].ch8");
	//loadProgram("kripod/programs/Clock Program [Bill Fisher, 1981].ch8");
	//loadProgram("kripod/programs/Chip8 emulator Logo [Garstyciuks].ch8");
	//loadProgram("kripod/programs/Delay Timer Test [Matthew Mikolay, 2010].ch8");
	//loadProgram("kripod/programs/Framed MK1 [GV Samways, 1980].ch8");
	//loadProgram("kripod/programs/Framed MK2 [GV Samways, 1980].ch8");
	//loadProgram("kripod/programs/Jumping X and O [Harry Kleinberg, 1977].ch8");
	//loadProgram("kripod/programs/Life [GV Samways, 1980].ch8");
	//loadProgram("kripod/programs/Minimal game [Revival Studios, 2007].ch8");
	//loadProgram("kripod/programs/Random Number Test [Matthew Mikolay, 2010].ch8");
	//loadProgram("kripod/programs/Fishie [Hap, 2005].ch8");
	//loadProgram("kripod/games/Animal Race [Brian Astle].ch8");
	//loadProgram("kripod/games/Astro Dodge [Revival Studios, 2008].ch8");mode = SUPERCHIP;
	//loadProgram("kripod/games/Coin Flipping [Carmelo Cortez, 1978].ch8");
	//loadProgram("kripod/games/Pong [Paul Vervalin, 1990].ch8");
	//file containing program hex
	FILE* rom = fopen(romFilename, "rb");

	//place starting at rom offset
	uint16_t address = OFFSET_ROM;
	char instruction = 0;
	uint8_t i = 0;
	if (rom)
	{
		fread(ram + OFFSET_ROM, 1, 0x0FFF - OFFSET_ROM, rom);
		fclose(rom);
	}

	//update PC
	PC = OFFSET_ROM;
}


void init()
{
	chip8Clear();
	loadConfig();
	loadFont();
	loadProgram();
}


void initDisplay()
{
	window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
}


//wave data as a ring data structure provided, loop through wavelength
WAVE_DATA_TYPE getSineWaveSample()
{
	static int wave_current_pos = 0;
	return wave[wave_current_pos++ % WAVE_LENGTH];
}


void audioCallback(void* userdata, Uint8* stream, int len)
{
	for (int n = 0; n < len; ++n)
	{
		stream[n] = getSineWaveSample();
	}
	//printf("i need %d bytes for buffer\n", len);
}

//initialize sdl audio
int initAudio()
{
	/*
	* Hint Audio:
	* you need to call SDL_OpenAudioDevice(...)
	*	no need obtained spec, but if not provided, the desired spec is modified
	*	allowing changes created problems
	* how it works (when callback is used)?
	*	when there is no audio data, SDL calls the audio callback
	*	that asks for spec.samples * spec.channels data (in bytes) to be filled in stream
	*	unfilled stream is silenced
	* Here I used a tone of 480 Hz,
	* for the 48000 sampling frequency, 48000/480 = 100 samples are needed
	*	to record one cycle of the sine wave.
	* To generate the one wave cycle (single channel),
	*	 maxSpecFormatSize*sin(2*pi*n/N) is used where N is 100 from before.
	* The audio callback fills the stream with data cycled through this wave cycle data.
	*	when end is reach, send data from the start.
	*/
	SDL_zero(spec);
	spec.freq = SAMPLING_FREQUENCY;
	spec.format = AUDIO_S8;
	spec.channels = 1;
	spec.samples = 512; //min samples per update
	spec.callback = audioCallback;
	spec.userdata = NULL;

	/*
	* paramter:
	* NULL	: device default or first available one
	* 0		: not a recording device
	* &spec	: desired audio spec we define
	* NULL	: obtained audio spec the system sets
	* flags	: allow system to change format as necessary
	*/

	dev = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
	if (!dev) printf("could not open audio device, %s\n", SDL_GetError());
	//else printf("Opened device: %s\n", SDL_GetAudioDeviceName(dev, 0));

	/*
	* sine wave generation
	* Asin(2*pi*n/N) where N is the number of samples, n is the position,
	* A single cycle(wavelength) in N samples.
	*
	*/

	for (int n = 0; n < WAVE_LENGTH; ++n)
	{
		wave[n] = (WAVE_DATA_TYPE)(sin(2 * M_PI * n / WAVE_LENGTH) * SDL_MAX_SINT8);
	}

	return 0;
}



void renderToSDLWindow()
{
	/* SDL Rendering tips
	*
	* to clear with a color:
	* SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF); //set color
	* SDL_RenderClear(renderer); //draw (need update to see change)
	*
	* to draw a solid rectangle(rectangle filled with color
	* SDL_Rect fillRect = { WIDTH / 4, HEIGHT / 4, WIDTH / 2, HEIGHT / 2 }; //set dimension
	* SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF); //set color
	* SDL_RenderFillRect(renderer, &fillRect); //draw (need update to see change)
	*
	* to draw a rectangle border only(outline)
	* SDL_Rect outlineRect = { WIDTH / 6, HEIGHT / 6, WIDTH * 2 / 3, HEIGHT * 2 / 3 };
	* SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);
	* SDL_RenderDrawRect(renderer, &outlineRect);
	*
	* to draw line
	* SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF);
	* SDL_RenderDrawLine(renderer, 0, HEIGHT / 2, WIDTH, HEIGHT / 2);
	*
	* to draw point
	* SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF);
	* SDL_RenderDrawPoint(renderer, WIDTH / 2, i);
	*
	* *NOTE* needed to render drawing (update screen)
	* SDL_RenderPresent(renderer);
	*
	*
	*/

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); //set color
	SDL_RenderClear(renderer); //draw (need update to see change)
	SDL_Rect fillRect;
	for (int y = 0; y < 32; ++y)
		for (int x = 0; x < 64; ++x)
		{
			if (vram[x][y])
			{
				SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF); //white
				fillRect = { x * scaleFactor,y * scaleFactor, scaleFactor, scaleFactor };
				SDL_RenderFillRect(renderer, &fillRect);
			}
		}
	SDL_RenderPresent(renderer);
}

uint16_t fetch()
{
	//get instruction
	uint16_t instruction = ram[PC] << 8 | ram[PC + 1]; //join MSB8 and LSB8 to get instruction16, PC incremented by 2
	PC += 2;
	//printf("fetched: %04x at %02x\n", instruction, PC - 2);
	return instruction;
}

void decodeandexecute(uint16_t instruction)
{
	//printf("VX:%2x, VY:%2x , VF:%2x\n", VX, VY, VF);

	switch (ITYPE)
	{
	case 0x0:
		switch (instruction)
		{
		case 0x00E0:
			//printf("00E0 display clear");
			clearDisplay();
			break;
		case 0x00EE:
			//printf("00EE return from subroutine");
			PC = pop();
			break;
		default:
			//printf("0NNN call");
			push(PC);
			PC = NNN;
			break;
		}
		break;
	case 0x1:
		//printf("1NNN goto NNN");
		PC = NNN;
		break;
	case 0x2:
		//printf("2NNN call subrouting at NNN");
		push(PC);
		PC = NNN;
		break;
	case 0x3:
		//printf("3XNN if VX==NN skip next instruction");
		if (VX == (NN)) PC += 2;
		break;
	case 0x4:
		//printf("4XNN if VX!=NN skip next instruction");
		if (VX != (NN)) PC += 2;
		break;
	case 0x5:
		//printf("5XY0 if VX==VY skip next instruction");
		if (VX == VY) PC += 2;
		break;
	case 0x6:
		//printf("6XNN set VX=NN");
		VX = NN;
		break;
	case 0x7:
		//printf("7XNN set VX=VX+NN, VF not changed");
		VX += NN;
		break;
	case 0x8:
		switch (instruction & 0x000F)
		{
		case 0x0:
			//printf("8XY0 VX = VY");
			VX = VY;
			break;
		case 0x1:
			//printf("8XY1 VX = VY | VY (or)");
			VX |= VY;
			if (mode & COSMACVIP) VF = 0;
			break;
		case 0x2:
			//printf("8XY2 VX = VY & VY (and)");
			VX &= VY;
			if (mode & COSMACVIP) VF = 0;
			break;
		case 0x3:
			//printf("8XY3 VX ^= VY (xor)");
			VX ^= VY;
			if (mode & COSMACVIP) VF = 0;
			break;
		case 0x4:
		{
			//printf("8XY4 VX = VX + VY (add), VF carry flag");
			uint16_t carrysum = (uint16_t)VX + (uint16_t)VY;
			VX = carrysum & 0x00FF;
			VF = uint8_t(carrysum >> 8) & 0x01;
			break;
		}
		case 0x5:
		{
			//printf("8XY5 VX = VX - VY (sub), VF burrow flag, 0 when burrow");
			int borrowdiff = ((uint16_t)VX | 0x0100) - (uint16_t)VY;
			VX = borrowdiff & 0x00FF;
			VF = (borrowdiff >> 8) & 0x01;
			break;
		}
		case 0x6:
			//printf("start %02x %02x %02x should be %02x %02x %02x\n", VX, VY, VF, VX >> 1, VY >> 1, VF >> 1);
			if (mode & COSMACVIP) //use VY
			{
				//printf("8XY6 VX = VY >> 1 (shiftr1), shift out to VF");
				uint8_t tmp = VY;
				VX = tmp >> 1;
				VF = tmp & 0x01;
			}
			if (mode & (CHIP48 | SUPERCHIP))
			{
				//ignore VY
				uint8_t tmp = VX;
				VX = tmp >> 1;
				VF = tmp & 0x01;
			}
			//printf("end %02x %02x %02x\n", VX, VY, VF); //std::cin.get();
			break;
		case 0x7:
		{
			//printf("8XY7 VX = VY - VX (sub reverse), VF burrow flag, 0 when burrow");
			uint16_t borrowdiff = ((uint16_t)VY | 0x0100) - (uint16_t)VX;
			VX = borrowdiff & 0x00FF;
			VF = (borrowdiff >> 8) & 0x01;
			break;
		}
		case 0xE:
			if (mode & COSMACVIP) //use VY
			{
				//printf("8XYE VX = VX << 1 (shiftl1), shift out to VF");
				uint8_t tmp = VY;
				VX = tmp << 1;
				VF = tmp >> 7;
			}
			if (mode & (CHIP48 | SUPERCHIP))
			{
				//ignore VY
				uint8_t tmp = VX;
				VX = tmp << 1;
				VF = tmp >> 7;
			}
			break;
		default:
			break;
		}
		break;
	case 0x9:
		//printf("9XY0 if VX != VY, skip next instruction");
		if (VX != VY) PC += 2;
		break;
	case 0xA:
		//printf("ANNN set I to NNN");
		I = NNN;
		break;
	case 0xB:
		//printf("BNNN jump to address (V0+NNN)");
		if (mode & SUPERCHIP) PC = NNN + VX;
		if (mode & COSMACVIP) PC = NNN + V0;


		break;
	case 0xC:
		//printf("CXNN set VX = rand0 & NN");
		VX = uint8_t(rand() % 0xFF) & NN;
		break;
	case 0xD:
		//printf("DXYN draw sprite at (VX,VY), width 8 and height N");
		draw(VX, VY, N) ? VF = 0x01 : VF = 0x0;
		break;
	case 0xE:
		switch (instruction & 0x00FF)
		{
		case 0x9E:
			//printf("EX9E skip next instruction if key stored in VX pressed");
			//printf("Instruction : %4x, %x key pressed? : %d, %x  \n", instruction, VX, keyIsPressed, pressedKeyHex);
			if (keyIsPressed && pressedKeyHex == VX)
			{
				PC += 2;
				//printf("ex9e key pressed. skipping next\n");
			}
			break;
		case 0xA1:
			//printf("EXA1 skip next instruction if key stored in VX NOT pressed");
			//printf("Instruction : %4x, %x key not pressed? : %d, %x  \n", instruction, VX, keyIsPressed, pressedKeyHex);
			if (!keyIsPressed || pressedKeyHex != VX)
			{
				PC += 2;
				//printf("exA1 key not pressed. skipping next\n");
			}
			break;
		default:
			break;
		}
		break;
	case 0xF:
		switch (instruction & 0x00FF)
		{
		case 0x07:
			//printf("FX07 set VX to value of delay timer");
			VX = timerDelay;
			break;
		case 0x0A:
			//printf("FX0A wait keypress, store in VX (blocking) ");
			/*
			* state machine:
			* //infinite loop until state machine complete
			* 00 : wait press,
			* |
			* |keyIsPressed
			* |
			* V
			* 01 : wait release / not wait press
			* |
			* |!keyIsPressed ===> store in VX, escape loop
			* |
			* V
			* 00
			*/
			//default state machine output : infinite loop
			PC -= 2;
			if (waitingKeyPress) //state 00
			{
				if (keyIsPressed) waitingKeyPress = false; //change state to 01
			}
			else //state 01 
			{ //not waiting key press / waiting key release
				if (!keyIsPressed) //key released
				{
					VX = pressedKeyHex; //store
					PC += 2; //break from infinite loop ,escape
					waitingKeyPress = true; // restore state machine , change back to 00
				}
			}
			break;
		case 0x15:
			//printf("FX15 set timerDelay = VX");
			timerDelay = VX;
			break;
		case 0x18:
			//printf("FX18 set timerSound = %d\n", VX);
			timerSound = VX;
			break;
		case 0x1E:
			//printf("FX1E I = I + VX , VF unchanged");
			I += VX;
			break;
		case 0x29:
			//printf("FX29 I = font[VX], set I to font sprite address stored in VX");
			I = font(VX);
			break;
		case 0x33:
			//printf("FX33 store BCD : 123 => I=1, I+1=2, I+2=3");
		{
			int value = VX;
			for (int i = 2; i >= 0;--i) //2 1 0
			{
				ram[I + i] = value % 10;
				value = value / 10;
			}
		}
		break;
		case 0x55:
			for (int i = 0; i <= X; ++i)
			{
				//printf("FX55 store V0 to VX in memory from address I as offset (no change I)");
				if (mode & COSMACVIP) ram[I++] = V[i];
				//printf("FX55 store V0 to VX in memory from address I as offset (increment I)");
				if (mode & (CHIP48 | SUPERCHIP)) ram[I + i] = V[i];
			}
			break;
		case 0x65:
			//printf("FX65 load V0 to VX from memory from address I as offset (no change in I)");
			for (int i = 0; i <= X; ++i)
			{
				//printf("FX65 fill V0 to VX from memory from address I as offset (no change I)");
				if (mode & COSMACVIP) V[i] = ram[I++];
				//printf("FX65 fill V0 to VX from memory from address I as offset (increment I)");
				if (mode & (CHIP48 | SUPERCHIP)) V[i] = ram[I + i];
			}
			break;
		default:
			break;
		}
	default:
		break;
	}
	//printf("VX:%2x, VY:%2x , VF:%2x\n", VX, VY, VF);
}


void emulate()
{
	//timers

	uint32_t currentMS = SDL_GetTicks();
	if ((currentMS - lastTimerUpdate) > FREQUENCY_TO_MILLIS(frequencyTimer))
	{
		lastTimerUpdate = currentMS;
		if (timerDelay)timerDelay--;
		if (timerSound)	SDL_PauseAudioDevice(dev, --timerSound ? 0 : 1);	//if timerSound register non zero, decrement and play if new value non zero. SoundTimer set to 1 at execution has no effect.
	}

	//display
	if ((currentMS - lastFrameUpdate) > FREQUENCY_TO_MILLIS(frameRate))
	{
		lastFrameUpdate = currentMS;
		renderToSDLWindow();
	}

	//run CPU cycle
	if (enableDelay && frequencyCPU)
	{
		if ((currentMS - lastCPUExecute) > FREQUENCY_TO_MILLIS(frequencyCPU))
		{
			lastCPUExecute = currentMS;
			decodeandexecute(fetch());
		}
	}
	else
	{
		decodeandexecute(fetch());
	}
	//render();
}



/*
 * handles keypress:
 * keys:
 * QWERTY   => Hex Value
 * 1 2 3 4	=>	1 2 3 C
 * Q W E R  =>	4 5 6 D
 * A S D F	=>	7 8 9 E
 * Z X C V	=>	A 0 B F
 * Scancode based, same key position even if different layout
*/

void handleKeyDown()
{
	keyIsPressed = true;
	switch (e.key.keysym.scancode)

	{
	case SDL_SCANCODE_1:
		pressedKeyHex = 0x1;
		break;
	case SDL_SCANCODE_2:
		pressedKeyHex = 0x2;
		break;
	case SDL_SCANCODE_3:
		pressedKeyHex = 0x3;
		break;
	case SDL_SCANCODE_4:
		pressedKeyHex = 0xC;
		break;
	case SDL_SCANCODE_Q:
		pressedKeyHex = 0x4;
		break;
	case SDL_SCANCODE_W:
		pressedKeyHex = 0x5;
		break;
	case SDL_SCANCODE_E:
		pressedKeyHex = 0x6;
		break;
	case SDL_SCANCODE_R:
		pressedKeyHex = 0xD;
		break;
	case SDL_SCANCODE_A:
		pressedKeyHex = 0x7;
		break;
	case SDL_SCANCODE_S:
		pressedKeyHex = 0x8;
		break;
	case SDL_SCANCODE_D:
		pressedKeyHex = 0x9;
		break;
	case SDL_SCANCODE_F:
		pressedKeyHex = 0xE;
		break;
	case SDL_SCANCODE_Z:
		pressedKeyHex = 0xA;
		break;
	case SDL_SCANCODE_X:
		pressedKeyHex = 0x0;
		break;
	case SDL_SCANCODE_C:
		pressedKeyHex = 0xB;
		break;
	case SDL_SCANCODE_V:
		pressedKeyHex = 0xF;
		break;
	case SDL_SCANCODE_ESCAPE:
		quit = true;
		break;
	case SDL_SCANCODE_BACKSPACE:
		init();
		break;
	default:
		break;
	}
}


void run()
{
	//loop unless quit event (Window closed or quit key press)
	while (!quit)
	{
		emulate();

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
				break;
			}
			else if (e.type == SDL_KEYDOWN)
			{
				handleKeyDown();
			}
			else if (e.type == SDL_KEYUP)
			{
				keyIsPressed = false;
			}
		}
	}
}





void push(uint16_t address)
{
	if (stackPointer < 255)
	{
		stack[++stackPointer] = address;
	}
}

uint16_t pop()
{
	if (stackPointer >= 0)
	{
		return stack[stackPointer--];
	}
	return -1;
}



void clearDisplay()
{
	for (int i = 0; i < 64; ++i)
		for (int j = 0; j < 32; ++j)
			vram[i][j] = 0;
}


bool setPixel(uint8_t x, uint8_t y, bool bit)
{
	vram[x][y] ^= bit; //xor with new bit
	return !vram[x][y] && bit; //return if flipped or not
}


bool draw(uint8_t x, uint8_t y, uint8_t num)
{
	//wrap around start coordinate
	x = x % CHIP8_DISPLAY_WIDTH;
	y = y % CHIP8_DISPLAY_HEIGHT;

	bool setToUnset = false; //set flag 0

	//get 8 bit pixels data, 
	//set at y pixel x to x+7 (increment x 1 at a time)
	//increment y upto y+N-1, get next 8 bit pixel data
	uint8_t pixel8 = 0;
	for (int n = 0; n < num && (y + n) < CHIP8_DISPLAY_HEIGHT; ++n) //clip if boundary exceeded
	{
		pixel8 = ram[I + n];
		for (int p = 0; p < 8 && (x + p) < CHIP8_DISPLAY_WIDTH; ++p)//clip if boundary exceeded
		{
			//if (setPixel(x + p, y + n, (pixel8 >> (7 - p)) & 0x1)) setToUnset = true;
			setToUnset |= setPixel(x + p, y + n, (pixel8 >> (7 - p)) & 0x1);
		}
	}
	return setToUnset;
}


void renderToConsole()
{
	printf("%02d|", 0);	for (uint8_t x = 0; x < 64; ++x) printf("%02d", x);	printf("\n"); //border
	printf("%02d|", 0);	for (uint8_t x = 0; x < 64; ++x) printf("__");	printf("\n"); //border
	for (uint8_t y = 0; y < 32; ++y) //print vram "**" for white, "  "black.
	{
		printf("%02d|", y);
		for (uint8_t x = 0; x < 64; ++x)
		{
			vram[x][y] ? printf("**") : printf("  ");
		}
		printf("|");
		printf("\n");
	}
	printf("%02d|", 0);	for (uint8_t x = 0; x < 64; ++x) printf("__");	printf("\n");
}


