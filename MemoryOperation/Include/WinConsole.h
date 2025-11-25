#pragma once
#include "Windows.h"
#include <consoleapi.h>
#include <consoleapi2.h>
#include <handleapi.h>
#include <processenv.h>
#include <consoleapi3.h>
#include <cstdio>
#include <WinBase.h>

class WindowsConsole
{
public:
	static HWND consoleWindowHandler;
	static bool Create();
	static void Destroy();
};