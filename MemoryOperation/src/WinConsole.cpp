#include "WinConsole.h"
#include <WinUser.h>

HWND WindowsConsole::consoleWindowHandler = nullptr;

bool WindowsConsole::Create() {

    if (!AllocConsole()) return false;
    // UTF-8 early: fixes mojibake when writing UTF-8 bytes
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    consoleWindowHandler = GetConsoleWindow();
    EnableMenuItem(GetSystemMenu(consoleWindowHandler, FALSE), SC_CLOSE, MF_GRAYED);
    DrawMenuBar(consoleWindowHandler);
    SetConsoleTitleA("Debugger");

    // Attach stdio to the console
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);

    // Enable ANSI (VT) without clobbering other flags
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!hOut || hOut == INVALID_HANDLE_VALUE) return false;

    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return false;

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    if (!SetConsoleMode(hOut, mode)) return false;

    return true;
}


void WindowsConsole::Destroy()
{

    if (consoleWindowHandler != nullptr)
    {
        fclose(stdout);
        fclose(stderr);
        FreeConsole();
        PostMessage(consoleWindowHandler, WM_CLOSE, 0, 0);
    }
}