#pragma once
#include <windows.h>

class CrashSuppressor {
private:
    static PVOID s_vectoredHandler;

public:
    static void Install();
    static void Remove();
    static bool IsInstalled();

private:
    static LONG NTAPI ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo);
};