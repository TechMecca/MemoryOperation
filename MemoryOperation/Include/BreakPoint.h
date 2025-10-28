#pragma once
#include <Windows.h>
#include <functional>

class Breakpoint {
public:
    // Use std::function for flexibility (lambdas, function pointers, etc.)
    typedef std::function<void(PCONTEXT)> Callback;

    // Main interface
    static bool Set(DWORD_PTR address, Callback callback);
    static bool Remove(DWORD_PTR address);
    static void RemoveAll();
    static bool IsSet(DWORD_PTR address);
    static int GetCount();

    // Utility functions
    static void PrintRegisters(PCONTEXT ctx);
    static DWORD GetRegisterValue(PCONTEXT ctx, const char* regName);

private:
    struct BPInfo {
        DWORD_PTR address;
        Callback callback;
        int drIndex;
        bool enabled;
        bool singleStepping;
    };

    static BPInfo s_breakpoints[4];
    static bool s_handlerInstalled;

    static LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS ex);
    static void InstallHandler();
    static int FindFreeDebugRegister();
    static void UpdateDebugRegisters();
    static void DisableBreakpoint(int drIndex);
    static void EnableBreakpoint(int drIndex);
    static void SingleStepInstruction(HANDLE hThread);
};