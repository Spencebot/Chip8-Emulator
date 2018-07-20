#include "stdafx.h"
#include "processor.h"
#include <windows.h>
#include <vector>
#include <iostream>
#include <string>
using namespace std;

processor::processor(vector<UINT16> romFile) {
	programToRun = romFile;
}

void processor::run()
{
	while (!exception) {
		UINT16 opCode = programToRun[pc - 0x200] * 256 + programToRun[pc - 0x1FF];
		UINT16 currentPc = pc;
		switch ((opCode & 0xF000) >> 12) {
		case 0x0:
			switch (opCode & 0x0FFF) {
			case 0x0E0: // Clear dsiplay - 00E0

				break;
			case 0x0EE: // Return - 00EE

				break;
			default: // deprecated Jump to machine code at nnn 0nnn

				break;
			}
			break;
		case 0x1: // Jump to address nnn - 1nnn
			pc = opCode & 0x0FFF;
			if (pc == currentPc){
				exception = true;
				errorText = "Explicit jump loop";
			}
			break;
		case 0x2: // Call subroutine at nnn - 2nnn
			if ((opCode & 0x0FFF) >= 0x200) {
				stack[sp] = pc;
				if (sp < sizeof(stack)/sizeof(stack[0])) {
					sp++;
				}
				else {
					exception = true;
					errorText = "";
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
			if (R[(opCode & 0x0F00) >> 8] == R[(opCode & 0x00F0)>>4]) {
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

				break;
			case 0x5: // Set Rx = Rx - Ry (Rf = not borrow) - 8xy5

				break;
			case 0x6: // Set Rx = shift right Rx (Rf gets shifted bit) - 8xy6

				break;
			case 0x7: // Set Rx = Ry - Rx (Rf = not borrow) - 8xy7

				break;
			case 0xE: // Set Rx = shift left Rx (Rf gets shifted bit) - 8xyE

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

			break;
		case 0xC: // Set Rx = Rand() AND kk - Cxkk

			break;
		case 0xD: // Draw n bytes at positon Rx, Ry - Dxyn

			break;
		case 0xE:
			switch (opCode & 0x00FF) {
			case 0x9E: // Skip next if key = Rx is pressed - Ex9E
				break;
			case 0xA1: // Skip next if key = Rx is not pressed - ExA1
				break;
			default: // invalid command

				break;
			}
			break;
		case 0xF:
			switch (opCode & 0x00FF) {
			case 0x07: // Set Rx = Delay Timer

				break;
			case 0x0A: // Set Rx = next key value

				break;
			case 0x15: // Set Delay Timer = Rx

				break;
			case 0x18: // Set Sound Timer = Rx

				break;
			case 0x1E: // Set I = I + Rx

				break;
			case 0x29: // Set I = location of sprite corresponding to value in Rx

				break;
			case 0x33: // Set binary coded decimal value of Rx to memory locations of I, I+1, and I+2

				break;
			case 0x55: // Set memory, starting at location of I, to R0-Rx

				break;
			case 0x65: //Set R0-Rx to values starting at memory location of I

				break;
			default: // invalid command

				break;
			}
			break;
		}
	}
	if (exception) {
		cout << errorText;
	}
	else {
		pc += 2;
	}
}

processor::~processor()
{
}
