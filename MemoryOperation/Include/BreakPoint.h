#pragma once
#include <windows.h>

class Breakpoint {
public:
    typedef void (*Callback)(PCONTEXT ctx);

    struct BPInfo {
        DWORD_PTR address = 0;
        Callback  callback = nullptr;
        int       index = -1;
        bool      enabled = false;
        bool      singleStepping = false;
    };

private:
    static BPInfo s_breakpoints[4];
    static bool s_handlerInstalled;
    static PVOID s_vehHandle;

    static void InstallHandler();
    static int FindFreeDebugRegister();
    static void ApplyBreakpointsToContext(CONTEXT& ctx);
    static void UpdateDebugRegisters();
    static void DisableBreakpoint(int drIndex);
    static void EnableBreakpoint(int drIndex);
    static LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS ex);

public:
    // Core control
    static bool Set(DWORD_PTR address, Callback callback);
    static bool Remove(DWORD_PTR address);
    static void RemoveAll();
    static bool IsSet(DWORD_PTR address);
    static int  GetCount();

    // Utility functions
    static void PrintRegisters(PCONTEXT ctx);
    static BPInfo* GetBreakpointInfo(DWORD_PTR address);
};