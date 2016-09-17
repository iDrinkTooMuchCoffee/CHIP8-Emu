#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2.h>


unsigned char chip8_fontset[80] =
{ 
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

typdef struct chip8
{
	FILE *game;
	unsigned short opcode;
	unsigned char memory[4096];
	unsigned char V[16];		// 15 8-bit registers
	unsigned char key[16];
	unsigned char display[64 * 32];		// 2048 pixel graphics
	unsigned short stack[16];
	int drawFlag = 0;
	unsigned short sp;	// Stack pointer
	unsigned short i;	// Index register
	unsigned short pc;	// Program counter
	unsigned char delay_timer;
	unsigned char sound_timer;
} C8;

/*
 * 0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
 * 0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
 * 0x200-0xFFF - Program ROM and work RAM
 */

void initialize()
{
	C8 CH8;
	CH8->pc = 0x200;
	CH8->opcode = CH8->memory[CH8->pc] << 8 | CH8->memory[CH8->pc + 1];
}

int emulateCycle()
{
	switch (CH8->opcode & 0xF000)
	{
		case 0x000:
			switch (CH8->opcode & 0x000F)
			{
				case 0x0000:	// 00E0: CLS - Clear the display
					for (int a = 1; a < 32 * 64; a++)
						CH8->display[a] = 0;
					CH8->drawFlag = 1;
				break;

				case 0x000E:	// 00EE: RET - Return from a subroutine
					CH8->sp--;	// Remove (pop) the top stack
					CH8->pc = CH8->stack[CH8->sp]; 	// Set the pc to the previous stack
					CH8->pc += 2;	// Chip-8 commands are 2 bytes
				break;
				default:
					printf("Wrong opcode: %d\n", CH8->opcode);
			}
		break;

		case 0x1000:	// 1NNN: Jumps to address NNN
			CH8->pc = CH8->opcode & 0x0FFF;
		break;

		case 0x2000:	// 2NNN: Calls subroutine at NNN
			CH8->sp++;	// Increment stack pointer
			CH8->sp = CH8->pc;	// Put the current program counter on top of the stack
			CH8->pc = CH8->opcode & 0x0FFF;	// Set the pc to NNN
		break;

		case 0x3000:	// 3XNN: SE vx, byte - Skip the next instruction if Vx == kk
			if (CH8->v[(CH8->opcode & 0x0F00) >> 8] == (CH8->opcode & 0x00FF))
				CH8->pc += 4;
			else
				CH8->pc += 2;
		break;
	
		case 0x4000:	// 3XNN: SNE Vx, byte- Skip the next instruction if Vx != kk
			if (CH8->v[(CH8->opcode & 0x0F00) >> 8] != (CH8->opcode & 0x00FF))
				CH8->pc += 4;
			else
				CH8->pc += 2;
		break;

		case 0x5000:	// 5XNN: SE Vx, Vy - Skip the next instruction if Vx = Vy
			if(CH8->v[(CH8->opcode & 0x0F00) >> 8] == CH8->v[(CH8->opcode & 0x00F0) >> 4])
				CH8->pc += 4;
			else
				CH8->pc += 2;
		break;

		case 0x6000:	// 6XNN: Sets VX to NN (put the value kk into register Vx)
			CH8->v[(CH8->opcode & 0xF00) >> 8] = (CH8->opcode & 0x00FF);
			CH8->pc += 2;
		break;

		case 0x7000:	// Adds NN to VX
			CH8->v[(CH8->opcode & 0x0F000) >> 8] += (CH8->opcode & 0x00FF);
			CH8->pc += 2;
		break;
	}

	return 0;
}

int main(int argc, char **argv)
{
	initialize();
	for (;;)
	{
		emulateCycle();
	}
}
