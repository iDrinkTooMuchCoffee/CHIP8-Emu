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
	//unsigned char display[64 * 32]; // 2048 pixel graphics
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
		//SDL_Delay(3000);
		//SDL_DestroyWindow(window);
		//SDL_Quit();
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
	//int i = 0;
	
	CH8->i      = 0;     // Reset index register
	CH8->sp     = 0;     // Reset stack pointer
	CH8->opcode = 0;     // Reset current opcode
	CH8->pc     = 0x200; // Program counter starts at 0x200

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
	memset(CH8->display, 0, sizeof(CH8->display));         // clear graphics
	memset(CH8->stack, 0, sizeof(CH8->stack));             // clear stack
	memset(CH8->V, 0, sizeof(CH8->V));                     // clear registers

	// Load fontset
	/*
	for (int i = 0; i < 80; ++i)
		CH8->memory[i] = chip8_fontset[i];	
		*/
}

void chip8_timers(C8 *CH8)
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
	CH8->opcode = CH8->memory[CH8->pc] << 8 | CH8->memory[CH8->pc + 1]; // Merge both bytes for opcode

	switch (CH8->opcode & 0xF000) // Decode opcode
	{
		case 0x0000:
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
		case 0x8000:
			switch (CH8->opcode & 0x000F)
			{
				case 0x0000: // 8XY0: Sets VX to the value of VY
					CH8->V[(CH8->opcode & 0x0F00) >> 8] = CH8->V[(CH8->opcode & 0x00F0) >> 4];
					CH8->pc += 2;
					break;

				case 0x0001: // 8XY1: Sets VX to VX or VY
					CH8->[(CH8->opcode & 0x0F00) >> 8] = CH8->V[(CH8->opcode & 0x0F00) >> 8] | CH8->V[(CH8->opcode & 0x00F0) >> 4];
					CH8->pc += 2;
					break;

				case 0x0002: // 8XY2: Sets VX to VX and VY
					CH8->[(CH8->opcode & 0x0F00) >> 8] = CH8->V[(CH8->opcode & 0x0F00) >> 8] & CH8->V[(CH8->opcode & 0x00F0) >> 4];
					CH8->pc += 2;
					break;

				case 0x0003: // 8XY3: Sets VX to VX xor VY
					CH8->[(CH8->opcode & 0x0F00) >> 8] = CH8->V[(CH8->opcode & 0x0F00) >> 8] ^ CH8->V[(CH8->opcode & 0x00F0) >> 4];
					CH8->pc += 2;
					break;

				case 0x0004: // 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there's not.
					if (CH8->V[(CH8->opcode & 0x00F0) >> 4] > (0xFF - CH8->V[(CH8->opcode & 0x0F00) >> 8]))
						CH8->V[0xF] = 1; // carry
					else
						CH8->V[0xF] = 0;
					CH8->V[(CH8->opcode & 0x0F00) >> 8] += CH8->V[(CH8->opcode & 0x00F0) >> 4];
					CH8->pc += 2;
					break;

				case 0x005: // 8XY5: Set Vx = Vx - Vy. If VX > VY then VF = 1, otherwise it's 0
					if (((int)CH8->V[(CH8->opcode & 0x0F00) >> 8] - (int)CH8->V[(CH8->opcode & 0x00F0) >> 4]) >= 0)
						CH8->V[0xF] = 1;
					else
						CH8->V[0xF] &= 0;
					CH8->V[(CH8->opcode & 0x0F00) >> 8] -= CH8->V[(CH8->opcode & 0x00F0) >> 4];
					CH8->pc += 2;
						
			}
			break;

		case 0xD000: // DXYN: Responsible for drawing to the display 
			// Get the position and height of the sprite
			unsigned short x = CH8->V[(CH8->opcode & 0x0F00) >> 8]; 
			unsigned short y = ch8->V[(opcode & 0X00F0) >> 4];
			unsigned short height = CH8->opcode & 0x000F;      // Pixel value
			unsigned short pixel;
			V[0xF] = 0;                                        // Reset register VF
			for (int yline = 0; yline < height; yline++)       // loop over each row
			{
				pixel = CH8->memory[CH8->i + yline];           // fetch pixel value from memory starting at I
				for (int xline = 0; xline < 8; xline++)        // loop over 8 bits of one row
				{
					if ((pixel & (0x80 >> xline)) != 0)        // Check if current evaluated pixel is 1. (0x80 >> xline scan through the byte, one bit at a time)
					{
						// Check if the pixel on display is set to 1, if so, register
						// collision by setting VF register
						if (CH8->display[(x + xline + ((y + yline) * 64))] == 1)
							CH8->V[0xF] = 1;
						CH8->display[x + xline + ((y + yline) * 64)] ^= 1; // Set pixel value 
					}
				}
			}
			CH8->drawFlag = 1; 
			CH8->pc += 2;
			break;

		default:
			printf("Unknown opcode: 0x%X\n", CH8->opcode);
	}

	chip8_timers(CH8); // Update timers
}

int main(int argc, char **argv)
{
	C8 *CH8;
	
	setup_graphics();
	setup_input();
	chip8_init(CH8);
	
	for (;;)
	{
		emulate_cycle(CH8);
	}
	
	return 0;
}
