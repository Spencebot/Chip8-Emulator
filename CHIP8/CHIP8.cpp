// CHIP8.cpp 

#include "stdafx.h"
#include "processor.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <bitset>
#include <SDL.h>
#undef main
using namespace std;

string getFile();
void parseROM(vector<uint8_t> binaryFile);
string parseOpCode(uint8_t, uint8_t);
void updateDisplay(SDL_Renderer *theRenderer, processor &aProcessor);
string to_hex(int, int);
string to_hex(int);
bool updateSdlEvents(const Uint8* &currentKeyStates);
int pixelSize = 15;
int milliSecondsBetweenUpdates = 17;

int main() {
	string romFilename;
	ifstream ROMfile;
	vector<uint8_t> ROMbinary;
	bool displayMissing = false;

	SDL_Window* window;
	SDL_Renderer* renderer;
	int posX = 5; int posY = 25; int sizeX = 64 * pixelSize; int sizeY = 32 * pixelSize;
	// Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		// Something failed, print error and exit.
		std::cout << " Failed to initialize SDL : " << SDL_GetError() << std::endl;
		return -1;
	}
	// Create and init the window
	window = SDL_CreateWindow("", posX, posY, sizeX, sizeY, 0);

	if (window == nullptr) {
		std::cout << "Failed to create window : " << SDL_GetError();
		return -1;
	}
	// Create and init the renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == nullptr) {
		std::cout << "Failed to create renderer : " << SDL_GetError();
		return -1;
	}
	// Render something
	// Set size of renderer to the same as window
	SDL_RenderSetLogicalSize(renderer, sizeX, sizeY);
	// Set color of renderer to black
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	// Clear the window and make it all black
	SDL_RenderClear(renderer);
	// Render the changes above 
	SDL_RenderPresent(renderer);

	// use explorer to get filename
	romFilename = getFile();
	// Check if file is already open
	if (!ROMfile.is_open()) {
		ROMfile.open(romFilename, ios::binary);
	}

	// store ROM into byte vector
	char tempByte;
	while (ROMfile.get(tempByte))
	{
		ROMbinary.push_back((uint8_t)tempByte);
	}
	ROMfile.close();
	parseROM(ROMbinary);

	// pad the end of the binary with 0s to make it approximately the useable size of the chip8
	// 4k memory, 512 for interpolator, 256(ish) where stack and disply info should go
	ROMbinary.resize(0xFFF - 0x200 - 0x100);

	processor myProcessor(ROMbinary);
	// use windows system time for display rate throttling
	SYSTEMTIME time;
	GetSystemTime(&time);
	LONG prevUpdateTime = (time.wSecond * 1000) + time.wMilliseconds;
	LONG currTime;

	while (!displayMissing) {
		// update the key state and set a report display is gone if user hits the "X"
		displayMissing = updateSdlEvents(myProcessor.currentKeyStates);
		// if there isnt a processor exception execute the next operation
		if (!myProcessor.exception) {
			// print the human readable operation and location
			cout << "0x" << uppercase << to_hex(myProcessor.getPc(), 3) << " "
				<< parseOpCode(ROMbinary[myProcessor.getPc() - 0x200], ROMbinary[myProcessor.getPc() - 0x1FF]) << endl;
			// execute command
			myProcessor.executeOperation();
		}
		// if there was an update to the display
		if (myProcessor.drawUpdate) {
			GetSystemTime(&time);
			currTime = (time.wSecond * 1000) + time.wMilliseconds;
			// wait until inter-frame time has elapsed
			while (abs(currTime - prevUpdateTime) < milliSecondsBetweenUpdates) {
				GetSystemTime(&time);
				currTime = (time.wSecond * 1000) + time.wMilliseconds;
			}
			prevUpdateTime = currTime;
			updateDisplay(renderer, myProcessor);
			myProcessor.drawUpdate = false;
		}
	}

	return 0;
}

void parseROM(vector<uint8_t> binaryData) {
	cout << internal // fill between the prefix and the number
		<< setfill('0'); // fill with 0s

	// loop over file stored in memory and ...
	for (uint16_t i = 0; i < (binaryData.size() - 1); i += 2) {
		// output memory location and ...
		cout << "0x" << uppercase << to_hex(0x200 + i, 3) << " "
			<< to_hex((binaryData[i] * 256 + binaryData[i + 1]), 4);
		// and translate the code to something readable
		cout << " : " << parseOpCode(binaryData[i], binaryData[i + 1]) << "\n";
	}
	system("pause");
}

string parseOpCode(uint8_t byte1, uint8_t byte2) {
	string returnString = "Default String";
	uint16_t opCode;
	opCode = byte1 * 256 + byte2;
	switch ((opCode & 0xF000) >> 12) {
	case 0x0:
		switch (opCode & 0x0FFF) {
		case 0x0E0: // Clear dsiplay - 00E0
			returnString = "CLS";
			break;
		case 0x0EE: // Return - 00EE
			returnString = "RET";
			break;
		default: // deprecated Jump to machine code at nnn 0nnn
			returnString = "Sprite or Deprecated OpCode";
			break;
		}
		break;
	case 0x1: // Jump to address nnn - 1nnn
		returnString = "JP " + to_hex((opCode & 0x0FFF), 3);
		break;
	case 0x2: // Call subroutine at nnn - 2nnn
		if ((opCode & 0x0FFF) >= 0x200) {
			returnString = "Call " + to_hex((opCode & 0x0FFF), 3);
		}
		else {
			returnString = "Sprite or Invalid Subroutine Location";
		}
		break;
	case 0x3: // Skip next if Rx == kk - 3xkk
		returnString = "SE R" + to_hex((opCode & 0x0F00) >> 8) + ", " + to_hex((opCode & 0x00FF));
		break;
	case 0x4: // Skip next if Rx != kk - 4xkk
		returnString = "SNE R" + to_hex((opCode & 0x0F00) >> 8) + ", " + to_hex((opCode & 0x00FF));
		break;
	case 0x5: // Skip next if Rx == Ry - 5xy0
		returnString = "SE R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
		break;
	case 0x6: // Set Rx = kkk - 6xkk
		returnString = "LD R" + to_hex((opCode & 0x0F00) >> 8) + ", " + to_hex((opCode & 0x00FF));
		break;
	case 0x7: // Set Rx = Rx + kk - 7xkk
		returnString = "ADD R" + to_hex((opCode & 0x0F00) >> 8) + ", " + to_hex((opCode & 0x00FF));
		break;
	case 0x8:
		switch (opCode & 0x000F) {
		case 0x0: // Set Rx = Ry - 8xy0
			returnString = "LD R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
			break;
		case 0x1: // Set Rx = Rx OR Ry - 8xy1
			returnString = "OR R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
			break;
		case 0x2: // Set Rx = Rx AND Ry - 8xy2
			returnString = "AND R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
			break;
		case 0x3: // Set Rx = Rx XOR Ry - 8xy3
			returnString = "XOR R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
			break;
		case 0x4: // Set Rx = Rx + Ry (Rf = carry bit) - 8xy4
			returnString = "ADD R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
			break;
		case 0x5: // Set Rx = Rx - Ry (Rf = not borrow) - 8xy5
			returnString = "SUB R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
			break;
		case 0x6: // Set Rx = shift right Ry (Rf gets shifted bit) - 8xy6
			returnString = "SHR R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
			break;
		case 0x7: // Set Rx = Ry - Rx (Rf = not borrow) - 8xy7
			returnString = "SUBN R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
			break;
		case 0xE: // Set Rx = shift left Ry (Rf gets shifted bit) - 8xyE
			returnString = "SHL R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
			break;
		default: // invalid command
			returnString = "Sprite or Undefined OpCode";
			break;
		}
		break;
	case 0x9: // Skip next if Rx != Ry - 9xy0
		returnString = "SNE R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4);
		break;
	case 0xA: // Set I = nnn - Annn
		returnString = "LD I, " + to_hex((opCode & 0x0FFF), 3);
		break;
	case 0xB: // Jump to address nnn + R0 - Bnnn
		returnString = "JP R0, " + to_hex((opCode & 0x0FFF), 3);
		break;
	case 0xC: // Set Rx = Rand() AND kk - Cxkk
		returnString = "RND R" + to_hex((opCode & 0x0F00) >> 8) + ", " + to_hex((opCode & 0x00FF));
		break;
	case 0xD: // Draw n bytes at positon Rx, Ry - Dxyn
		returnString = "DRW R" + to_hex((opCode & 0x0F00) >> 8) + ", R" + to_hex((opCode & 0x00F0) >> 4) + ", " + to_hex((opCode & 0x000F));
		break;
	case 0xE:
		switch (opCode & 0x00FF) {
		case 0x9E: // Skip next if key = Rx is pressed - Ex9E
			returnString = "SKP R" + to_hex((opCode & 0x0F00) >> 8);
			break;
		case 0xA1: // Skip next if key = Rx is not pressed - ExA1
			returnString = "SKNP R" + to_hex((opCode & 0x0F00) >> 8);
			break;
		default: // invalid command
			returnString = "Sprite or Undefined OpCode";
			break;
		}
		break;
	case 0xF:
		switch (opCode & 0x00FF) {
		case 0x07: // Set Rx = Delay Timer
			returnString = "LD R" + to_hex((opCode & 0x0F00) >> 8) + ", DT";
			break;
		case 0x0A: // Set Rx = next key value
			returnString = "LD R" + to_hex((opCode & 0x0F00) >> 8) + ", K";
			break;
		case 0x15: // Set Delay Timer = Rx
			returnString = "LD DT, R" + to_hex((opCode & 0x0F00) >> 8);
			break;
		case 0x18: // Set Sound Timer = Rx
			returnString = "LD ST, R" + to_hex((opCode & 0x0F00) >> 8);
			break;
		case 0x1E: // Set I = I + Rx
			returnString = "ADD I, R" + to_hex((opCode & 0x0F00) >> 8);
			break;
		case 0x29: // Set I = location of sprite corresponding to value in Rx
			returnString = "LD F, R" + to_hex((opCode & 0x0F00) >> 8);
			break;
		case 0x33: // Set binary coded decimal value of Rx to memory locations of I, I+1, and I+2
			returnString = "LD B, R" + to_hex((opCode & 0x0F00) >> 8);
			break;
		case 0x55: // Set memory, starting at location of I, to R0-Rx
			returnString = "LD [I], R" + to_hex((opCode & 0x0F00) >> 8);
			break;
		case 0x65: //Set R0-Rx to values starting at memory location of I
			returnString = "LD R" + to_hex((opCode & 0x0F00) >> 8) + ", [I]";
			break;
		default: // invalid command
			returnString = "Sprite or Undefined OpCode";
			break;
		}
		break;
	}
	return returnString;
}

void updateDisplay(SDL_Renderer *theRenderer, processor &aProcessor) {
	// Set color of renderer to black
	SDL_SetRenderDrawColor(theRenderer, 0, 0, 0, 255);
	// Clear the window and make it all black
	SDL_RenderClear(theRenderer);
	for (uint16_t row = 0; row < aProcessor.display.size(); row++) {
		bitset<64> tempRow(aProcessor.display[row]);
		for (uint16_t col = 0; col < tempRow.size(); col++) {
			// Create a rectangle
			SDL_Rect r;
			r.x = (tempRow.size() - col - 1) * pixelSize; r.y = row * pixelSize; r.w = pixelSize; r.h = pixelSize;
			if (tempRow[col]) {
				// Change color to white
				SDL_SetRenderDrawColor(theRenderer, 255, 255, 255, 255);
				// Render our SDL_Rect
				SDL_RenderFillRect(theRenderer, &r);
			}
		}
	}
	// Render the changes above
	SDL_RenderPresent(theRenderer);
}

bool updateSdlEvents(const Uint8* &currentKeyStates) {
	SDL_Event e;
	bool quit = false;
	while (SDL_PollEvent(&e) != 0) {
		SDL_PollEvent(&e);
		if (e.type == SDL_QUIT) { quit = true; }
	}
	currentKeyStates = SDL_GetKeyboardState(NULL);
	return quit;
}
string to_hex(int i)
{
	string returnString;
	returnString = to_hex(i, 1);
	return returnString;
}

string to_hex(int i, int width)
{
	stringstream stream;
	stream << uppercase << setfill('0') << setw(width)
		<< hex << i;
	return stream.str();
}

string getFile() {
	OPENFILENAMEW file;
	wchar_t szFile[100];
	ZeroMemory(&file, sizeof(file));
	file.lStructSize = sizeof(file);
	file.hwndOwner = NULL;
	file.lpstrFile = szFile;
	file.lpstrFile[0] = '\0';
	file.nMaxFile = sizeof(szFile);
	file.lpstrFilter = L"Chip8\0*.ch8\0All\0*.*\0";
	file.nFilterIndex = 1;
	file.lpstrFileTitle = NULL;
	file.nMaxFileTitle = 0;
	file.lpstrInitialDir = NULL;
	file.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	GetOpenFileNameW(&file);
	wstring ws(file.lpstrFile);
	string filename(ws.begin(), ws.end());
	return filename;
}
