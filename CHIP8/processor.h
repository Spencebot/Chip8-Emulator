#pragma once
#include "stdafx.h"
#include "processor.h"
#include <windows.h>
#include <vector>
#include <string>
using namespace std;

class processor
{
private:
	vector<UINT16> programToRun;
	UINT16 pc = 0x200;
	UINT16 stack[16] = {0};
	UINT8 sp = 0;
	UINT8 display[64 * 32] = {0};
	UINT8 I = 0;
	UINT8 R[16] = {0};
	UINT8 DT = 0;
	UINT8 ST = 0;
	bool exception;
	string errorText;

public:
	processor(vector<UINT16> romFile);

	void run();

	~processor();
};

