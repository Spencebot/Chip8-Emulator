#include "stdafx.h"
#include "processor.h"
#include <vector>
#include <iostream>
#include <string>
#include <array>
using namespace std;

processor::processor(vector<uint8_t> romFile) {
	programToRun = romFile;
}

void processor::run()
{
	while (!exception) {
		uint16_t opCode = programToRun[pc - 0x200] * 256 + programToRun[pc - 0x1FF];
		uint16_t currentPc = pc;
		int result;
		switch ((opCode & 0xF000) >> 12) {
		case 0x0:
			switch (opCode & 0x0FFF) {
			case 0x0E0: // Clear dsiplay - 00E0
				display.fill(0);
				break;
			case 0x0EE: // Return - 00EE
				pc = stack[sp] - 2;
				if (sp > 0) {
					sp--;
				}
				else {
					exception = true;
					errorText = "Attempted to return with an empty stack";
				}
				break;
			default: // deprecated Jump to machine code at nnn 0nnn
				exception = true;
				errorText = "Attempted to access machine code that doesn't exist on this platform";
				break;
			}
			break;
		case 0x1: // Jump to address nnn - 1nnn
			if ((opCode & 0x0FFF) == pc) {
				exception = true;
				errorText = "Explicit jump loop";
			}
			pc = opCode & 0x0FFF;
			break;
		case 0x2: // Call subroutine at nnn - 2nnn
			if ((opCode & 0x0FFF) >= 0x200) {
				sp++;
				if (sp <= sizeof(stack) / sizeof(stack[0])) {
					stack[sp] = pc;
				}
				else {
					exception = true;
					errorText = "Reached maximum stack depth";
				}
				pc = (opCode & 0x0FFF) - 2;
			}
			else {
				exception = true;
				errorText = "Subroutine called at invalid memory location";
			}
			break;
		case 0x3: // Skip next if Rx == kk - 3xkk
			if (R[(opCode & 0x0F00) >> 8] == (opCode & 0x00FF)) {
				pc += 2;
			}
			break;
		case 0x4: // Skip next if Rx != kk - 4xkk
			if (R[(opCode & 0x0F00) >> 8] != (opCode & 0x00FF)) {
				pc += 2;
			}
			break;
		case 0x5: // Skip next if Rx == Ry - 5xy0
			if (R[(opCode & 0x0F00) >> 8] == R[(opCode & 0x00F0) >> 4]) {
				pc += 2;
			}
			break;
		case 0x6: // Set Rx = kkk - 6xkk
			R[(opCode & 0x0F00) >> 8] = (opCode & 0x00FF);
			break;
		case 0x7: // Set Rx = Rx + kk - 7xkk
			R[(opCode & 0x0F00) >> 8] = R[(opCode & 0x0F00) >> 8] + (opCode & 0x00FF);
			// overflow?
			break;
		case 0x8:
			switch (opCode & 0x000F) {
			case 0x0: // Set Rx = Ry - 8xy0
				R[(opCode & 0x0F00) >> 8] = R[(opCode & 0x00F0) >> 4];
				break;
			case 0x1: // Set Rx = Rx OR Ry - 8xy1
				R[(opCode & 0x0F00) >> 8] = R[(opCode & 0x0F00) >> 8] | R[(opCode & 0x00F0) >> 4];
				break;
			case 0x2: // Set Rx = Rx AND Ry - 8xy2
				R[(opCode & 0x0F00) >> 8] = R[(opCode & 0x0F00) >> 8] & R[(opCode & 0x00F0) >> 4];
				break;
			case 0x3: // Set Rx = Rx XOR Ry - 8xy3
				R[(opCode & 0x0F00) >> 8] = R[(opCode & 0x0F00) >> 8] ^ R[(opCode & 0x00F0) >> 4];
				break;
			case 0x4: // Set Rx = Rx + Ry (Rf = carry bit) - 8xy4
				result = R[(opCode & 0x0F00) >> 8] + R[(opCode & 0x00F0) >> 4];
				R[0xF] = 0;
				if (result > 0xFF) R[0xF] = 1;
				R[(opCode & 0x0F00) >> 8] = (result && 0xFF);
				break;
			case 0x5: // Set Rx = Rx - Ry (Rf = not borrow) - 8xy5
				result = R[(opCode & 0x0F00) >> 8] - R[(opCode & 0x00F0) >> 4];
				R[0xF] = 0;
				if (result > 0) R[0xF] = 1;
				R[(opCode & 0x0F00) >> 8] = (result && 0xFF);
				break;
			case 0x6: // Set Rx = shift right Ry (Rf gets shifted bit) - 8xy6
				R[0xF] = R[(opCode & 0x00F0) >> 4] & 0x01;
				R[(opCode & 0x0F00) >> 8] = R[(opCode & 0x00F0) >> 4] >> 1;
				break;
			case 0x7: // Set Rx = Ry - Rx (Rf = not borrow) - 8xy7
				result = R[(opCode & 0x00F0) >> 4] - R[(opCode & 0x0F00) >> 8];
				R[0xF] = 0;
				if (result > 0) R[0xF] = 1;
				R[(opCode & 0x0F00) >> 8] = (result && 0xFF);
				break;
			case 0xE: // Set Rx = shift left Ry (Rf gets shifted bit) - 8xyE
				R[0xF] = R[(opCode & 0x00F0) >> 4] & 0x80;
				R[(opCode & 0x0F00) >> 8] = R[(opCode & 0x00F0) >> 4] << 1;
				break;
			default: // invalid command
				exception = true;
				errorText = "Unrecognized operation: " + opCode;
				break;
			}
			break;
		case 0x9: // Skip next if Rx != Ry - 9xy0
			if (R[(opCode & 0x0F00) >> 8] != R[(opCode & 0x00F0) >> 4]) {
				pc += 2;
			}
			break;
		case 0xA: // Set I = nnn - Annn
			I = opCode & 0x0FFF;
			break;
		case 0xB: // Jump to address nnn + R0 - Bnnn
			if (((opCode & 0x0FFF) + R[0]) == pc) {
				exception = true;
				errorText = "Explicit jump loop";
			}
			pc = (opCode & 0x0FFF) + R[0];
			break;
		case 0xC: // Set Rx = Rand() AND kk - Cxkk
			R[(opCode & 0x0F00) >> 8] = ((rand() & 0xFF) & (opCode & 0x00FF));
			break;
		case 0xD: // Draw n bytes at positon Rx, Ry - Dxyn
			for (int n = 0; n < (opCode & 0x000F); n++) {
				uint64_t tempSpriteRow;
				R[0xF] = 0;
				if (I < 0) { // if it is a predefined sprite
					tempSpriteRow = sprites[(-1 * I - 1) * 5 + n];
				}
				else { // load memory to display
					tempSpriteRow = programToRun[(I-0x200) + n];
				}
				// shift the row to column 0 and then down to location x
				tempSpriteRow = tempSpriteRow << (64 - 8) >> ((opCode & 0x0F00) >> 8);
				// Set RF if collision on this row
				if ((display[((opCode & 0x00F0) >> 4) + n] & tempSpriteRow) != 0) R[0xF] = 1;
				// display(row) XOR sprite
				display[((opCode & 0x00F0) >> 4) + n] ^= tempSpriteRow;
			}
			break;
		case 0xE:
			switch (opCode & 0x00FF) {
			case 0x9E: // Skip next if key = Rx is pressed - Ex9E

				break;
			case 0xA1: // Skip next if key = Rx is not pressed - ExA1

				break;
			default: // invalid command
				exception = true;
				errorText = "Unrecognized operation: " + opCode;
				break;
			}
			break;
		case 0xF:
			switch (opCode & 0x00FF) {
			case 0x07: // Set Rx = Delay Timer
				R[(opCode & 0x0F00) >> 8] = DT;
				break;
			case 0x0A: // Set Rx = next key value

				break;
			case 0x15: // Set Delay Timer = Rx
				DT = R[(opCode & 0x0F00) >> 8];
				break;
			case 0x18: // Set Sound Timer = Rx
				ST = R[(opCode & 0x0F00) >> 8];
				break;
			case 0x1E: // Set I = I + Rx
				I += R[(opCode & 0x0F00) >> 8];
				break;
			case 0x29: // Set I = location of sprite corresponding to value in Rx
				I = -1 * R[(opCode & 0x0F00) >> 8];
				break;
			case 0x33: // Set binary coded decimal value of Rx to memory locations of I, I+1, and I+2
				if (I > 0x200) {
					programToRun[(I-0x200) + 2] = R[(opCode & 0x0F00) >> 8] % 10;
					programToRun[(I - 0x200) + 1] = ((R[(opCode & 0x0F00) >> 8] - programToRun[(I - 0x200) + 2]) % 100) / 10;
					programToRun[(I - 0x200)] = (R[(opCode & 0x0F00) >> 8] - 10 * programToRun[(I - 0x200) + 1] - programToRun[(I - 0x200) + 2]) / 100;
				}
				// else { do I need to implement a user being able to overwrite default sprites?}
				break;
			case 0x55: // Set memory, starting at location of I, to R0-Rx
				for (int n = 0; n <= (opCode & 0x0F00) >> 8; n++) {
					programToRun[(I - 0x200) + n] = R[(opCode & 0x0F00) >> 8];
				}
				break;
			case 0x65: //Set R0-Rx to values starting at memory location of I
				for (int n = 0; n <= (opCode & 0x0F00) >> 8; n++) {
					R[(opCode & 0x0F00) >> 8] = programToRun[(I - 0x200) + n];
				}
				break;
			default: // invalid command
				exception = true;
				errorText = "Unrecognized operation: " + opCode;
				break;
			}
			break;
		}

		if (exception) cout << errorText;
		else {
			pc += 2;
			if (DT > 0) DT--;
			if (DT > 0) DT--;
		}
	}
}

processor::~processor()
{
}
