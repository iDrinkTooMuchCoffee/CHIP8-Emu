#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define memsize 4096
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 320

typedef struct chip8
{
	FILE *game;
	unsigned short opcode;
	unsigned char memory[4096];
	unsigned char V[16];            // 15 8-bit registers
	unsigned char key[16];
	unsigned char display[64 * 32]; // 2048 pixel graphics
	unsigned short stack[16];
	int drawFlag;
	unsigned short sp;              // Stack pointer
	unsigned short i;               // Index register
	unsigned short pc;              // Program counter
	unsigned char delay_timer;
	unsigned char sound_timer;
} C8;

/*
 * 0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
 * 0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
 * 0x200-0xFFF - Program ROM and work RAM
 */

void setup_graphics()
{
	SDL_Window *window;
	SDL_Init(SDL_INIT_EVERYTHING);
	
	window = SDL_CreateWindow("Chip8 window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);

	if (window == NULL)
		printf("Could not create window: SDL_ERROR: %s\n", SDL_GetError());
	else
	{
		// Just here to make sure the window works
		SDL_Delay(3000);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
}

void setup_input()
{
	Uint8 * keys;
	SDL_Event event;
	SDL_Init(SDL_INIT_EVERYTHING);
}

void chip8_init(C8 *CH8)
{
	char game_name[100];
	int i;
	
	printf("Enter name of game: ");
	scanf("%s", game_name);

	CH8->game = fopen(game_name, "rb");
	if (!CH8->game)
	{
		printf("Invalid game name");
		fflush(stdin);
		getchar();
		exit(1);
	}

	fread(CH8->memory+0x200, 1, memsize-0x200, CH8->game); // load game into memory
/*
	for (i = 0; i < 80; ++i)
		CH->memory[i] = chip8_fontset[i];                  // load fontset into memory
*/
	memset(CH8->display, 0, sizeof(CH8->display));         // clear graphics
	memset(CH8->stack, 0, sizeof(CH8->stack));             // clear stack
	memset(CH8->V, 0, sizeof(CH8->V));                     // clear chip8 register

	CH8->sp &= 0;
	CH8->pc = 0x200;
	CH8->opcode = 0x200;
}

void timers(C8 * CH8)
{
	if (CH8->delay_timer > 0)
		CH8->delay_timer--;
	if (CH8->sound_timer > 0)
		CH8->sound_timer--;
	if (CH8->sound_timer != 0)
		printf("%c", 7);
}

void emulate_cycle(C8 *CH8)
{
	Uint8 * keys;
	int y, x, vx, vy, times, i;
	unsigned height, pixel;


	switch (CH8->opcode & 0xF000)
	{
		case 0x000:
			switch (CH8->opcode & 0x000F)
			{
				case 0x0000:                                   // 00E0: CLS - Clear the display
					for (int a = 1; a < 32 * 64; a++)
						CH8->display[a] = 0;
					CH8->drawFlag = 1;
					break;

				case 0x000E:                                   // 00EE: RET - Return from a subroutine
					CH8->sp--;                                 // pop the top of the stack
					CH8->pc = CH8->stack[CH8->sp];             // Set the pc to the previous stack
					CH8->pc += 2;                              // Chip-8 commands are 2 bytes
					break;
				default:
					printf("Wrong opcode: %d\n", CH8->opcode);
			}
			break;

		case 0x1000:                        // 1NNN: Jumps to address NNN
			CH8->pc = CH8->opcode & 0x0FFF;
			break;

		case 0x2000:                        // 2NNN: Calls subroutine at NNN
			CH8->sp++;                      // Increment stack pointer
			CH8->sp = CH8->pc;              // Put the current program counter on top of the stack
			CH8->pc = CH8->opcode & 0x0FFF; // Set the pc to NNN
			break;

		case 0x3000:	// 3XNN: SE vx, byte - Skip the next instruction if Vx == kk
			if (CH8->V[(CH8->opcode & 0x0F00) >> 8] == (CH8->opcode & 0x00FF))
				CH8->pc += 4;
			else
				CH8->pc += 2;
			break;
	
		case 0x4000:	// 3XNN: SNE Vx, byte- Skip the next instruction if Vx != kk
			if (CH8->V[(CH8->opcode & 0x0F00) >> 8] != (CH8->opcode & 0x00FF))
				CH8->pc += 4;
			else
				CH8->pc += 2;
			break;

		case 0x5000:	// 5XNN: SE Vx, Vy - Skip the next instruction if Vx = Vy
			if(CH8->V[(CH8->opcode & 0x0F00) >> 8] == CH8->V[(CH8->opcode & 0x00F0) >> 4])
				CH8->pc += 4;
			else
				CH8->pc += 2;
			break;

		case 0x6000:	// 6XNN: Sets VX to NN (put the value kk into register Vx)
			CH8->V[(CH8->opcode & 0xF00) >> 8] = (CH8->opcode & 0x00FF);
			CH8->pc += 2;
			break;

		case 0x7000:	// Adds NN to VX
			CH8->V[(CH8->opcode & 0x0F000) >> 8] += (CH8->opcode & 0x00FF);
			CH8->pc += 2;
			break;
	}
}

int main(int argc, char **argv)
{
	C8 *CH8;
	
	setup_graphics();
	setup_input();
	chip8_init(&CH8);
	
	for (;;)
	{
		emulate_cycle(&CH8); // emulate one cycle
	}
	
	return 0;
}
