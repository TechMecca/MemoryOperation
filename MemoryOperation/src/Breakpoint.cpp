#include "Breakpoint.h"
#include <stdio.h>

// Static member initialization
Breakpoint::BPInfo Breakpoint::s_breakpoints[4] = {};
bool Breakpoint::s_handlerInstalled = false;
PVOID Breakpoint::s_vehHandle = nullptr;

// Install the vectored exception handler (once)
void Breakpoint::InstallHandler() {
    if (!s_handlerInstalled) {
        s_vehHandle = AddVectoredExceptionHandler(1, ExceptionHandler);
        s_handlerInstalled = (s_vehHandle != nullptr);
    }
}

// Find a free debug register (DR0-DR3)
int Breakpoint::FindFreeDebugRegister() {
    for (int i = 0; i < 4; i++) {
        if (!s_breakpoints[i].enabled) {
            return i;
        }
    }
    return -1;
}

// Apply all active breakpoints to a given context
void Breakpoint::ApplyBreakpointsToContext(CONTEXT& ctx) {
    ctx.Dr7 = 0;

    for (int i = 0; i < 4; i++) {
        if (s_breakpoints[i].enabled) {
            switch (i) {
            case 0: ctx.Dr0 = s_breakpoints[i].address; break;
            case 1: ctx.Dr1 = s_breakpoints[i].address; break;
            case 2: ctx.Dr2 = s_breakpoints[i].address; break;
            case 3: ctx.Dr3 = s_breakpoints[i].address; break;
            }

            // Set DR7 bits for execution breakpoint (RW=00, LEN=00, L=1)
            // L bit (local enable) is at position (i * 2)
            ctx.Dr7 |= (1ULL << (i * 2));

            // Condition bits: RW (bits 16-17, 20-21, 24-25, 28-29) = 00 for execution
            // LEN bits (bits 18-19, 22-23, 26-27, 30-31) = 00 for 1 byte
            // These are already 0, so no need to set them
        }
    }

    // Enable general and local exact breakpoint detection
    ctx.Dr7 |= 0x300;
}

// Update debug registers in the current thread
void Breakpoint::UpdateDebugRegisters() {
    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    HANDLE hThread = GetCurrentThread();
    if (GetThreadContext(hThread, &ctx)) {
        ApplyBreakpointsToContext(ctx);
        SetThreadContext(hThread, &ctx);
    }
}

// Disable a specific breakpoint by index
void Breakpoint::DisableBreakpoint(int drIndex) {
    if (drIndex < 0 || drIndex >= 4) return;

    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    HANDLE hThread = GetCurrentThread();

    if (GetThreadContext(hThread, &ctx)) {
        // Clear the local enable bit for this DR
        ctx.Dr7 &= ~(1ULL << (drIndex * 2));
        SetThreadContext(hThread, &ctx);
    }
}

// Enable a specific breakpoint by index
void Breakpoint::EnableBreakpoint(int drIndex) {
    if (drIndex < 0 || drIndex >= 4) return;
    UpdateDebugRegisters();
}

// Main exception handler
LONG WINAPI Breakpoint::ExceptionHandler(PEXCEPTION_POINTERS ex) {
    DWORD exceptionCode = ex->ExceptionRecord->ExceptionCode;

    // Handle single-step exception (for re-enabling breakpoints)
    if (exceptionCode == EXCEPTION_SINGLE_STEP) {
        PCONTEXT ctx = ex->ContextRecord;

        // Check if this was our single-step
        bool ourStep = false;
        for (int i = 0; i < 4; i++) {
            if (s_breakpoints[i].singleStepping) {
                s_breakpoints[i].singleStepping = false;
                ourStep = true;
            }
        }

        if (ourStep) {
            // Re-enable all breakpoints
            ApplyBreakpointsToContext(*ctx);
            ctx->EFlags &= ~0x100; // Clear trap flag
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        // Check which debug register triggered
        DWORD_PTR dr6 = ctx->Dr6;

        for (int i = 0; i < 4; i++) {
            if ((dr6 & (1ULL << i)) && s_breakpoints[i].enabled) {
                DWORD_PTR hitAddr;
#ifdef _WIN64
                hitAddr = ctx->Rip;
#else
                hitAddr = ctx->Eip;
#endif

                if (hitAddr == s_breakpoints[i].address) {
                    // Call the user callback
                    if (s_breakpoints[i].callback) {
                        s_breakpoints[i].callback(ctx);
                    }

                    // Temporarily disable this breakpoint and enable single-step
                    switch (i) {
                    case 0: ctx->Dr0 = 0; break;
                    case 1: ctx->Dr1 = 0; break;
                    case 2: ctx->Dr2 = 0; break;
                    case 3: ctx->Dr3 = 0; break;
                    }
                    ctx->Dr7 &= ~(1ULL << (i * 2));

                    // Set trap flag for single-step
                    ctx->EFlags |= 0x100;
                    s_breakpoints[i].singleStepping = true;

                    // Clear the debug status register
                    ctx->Dr6 = 0;

                    return EXCEPTION_CONTINUE_EXECUTION;
                }
            }
        }

        // Clear Dr6 to acknowledge the exception
        ctx->Dr6 = 0;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

// Set a hardware breakpoint
bool Breakpoint::Set(DWORD_PTR address, Callback callback) {
    if (!callback || address == 0) {
        return false;
    }

    // Check if already set
    if (IsSet(address)) {
        return false;
    }

    InstallHandler();

    int drIndex = FindFreeDebugRegister();
    if (drIndex < 0) {
        return false; // No free debug registers
    }

    s_breakpoints[drIndex].address = address;
    s_breakpoints[drIndex].callback = callback;
    s_breakpoints[drIndex].index = drIndex;
    s_breakpoints[drIndex].enabled = true;
    s_breakpoints[drIndex].singleStepping = false;

    UpdateDebugRegisters();
    return true;
}

// Remove a hardware breakpoint
bool Breakpoint::Remove(DWORD_PTR address) {
    for (int i = 0; i < 4; i++) {
        if (s_breakpoints[i].enabled && s_breakpoints[i].address == address) {
            s_breakpoints[i].enabled = false;
            s_breakpoints[i].address = 0;
            s_breakpoints[i].callback = nullptr;
            s_breakpoints[i].index = -1;
            s_breakpoints[i].singleStepping = false;

            UpdateDebugRegisters();
            return true;
        }
    }
    return false;
}

// Remove all breakpoints
void Breakpoint::RemoveAll() {
    for (int i = 0; i < 4; i++) {
        s_breakpoints[i].enabled = false;
        s_breakpoints[i].address = 0;
        s_breakpoints[i].callback = nullptr;
        s_breakpoints[i].index = -1;
        s_breakpoints[i].singleStepping = false;
    }
    UpdateDebugRegisters();
}

// Check if a breakpoint is set at an address
bool Breakpoint::IsSet(DWORD_PTR address) {
    for (int i = 0; i < 4; i++) {
        if (s_breakpoints[i].enabled && s_breakpoints[i].address == address) {
            return true;
        }
    }
    return false;
}

// Get count of active breakpoints
int Breakpoint::GetCount() {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (s_breakpoints[i].enabled) {
            count++;
        }
    }
    return count;
}

// Utility: Print register values from context
void Breakpoint::PrintRegisters(PCONTEXT ctx) {
#ifdef _WIN64
    printf("RAX: 0x%016llX  RBX: 0x%016llX\n", ctx->Rax, ctx->Rbx);
    printf("RCX: 0x%016llX  RDX: 0x%016llX\n", ctx->Rcx, ctx->Rdx);
    printf("RSI: 0x%016llX  RDI: 0x%016llX\n", ctx->Rsi, ctx->Rdi);
    printf("RBP: 0x%016llX  RSP: 0x%016llX\n", ctx->Rbp, ctx->Rsp);
    printf("RIP: 0x%016llX\n", ctx->Rip);
    printf("R8:  0x%016llX  R9:  0x%016llX\n", ctx->R8, ctx->R9);
    printf("R10: 0x%016llX  R11: 0x%016llX\n", ctx->R10, ctx->R11);
    printf("R12: 0x%016llX  R13: 0x%016llX\n", ctx->R12, ctx->R13);
    printf("R14: 0x%016llX  R15: 0x%016llX\n", ctx->R14, ctx->R15);
#else
    printf("EAX: 0x%08X  EBX: 0x%08X\n", ctx->Eax, ctx->Ebx);
    printf("ECX: 0x%08X  EDX: 0x%08X\n", ctx->Ecx, ctx->Edx);
    printf("ESI: 0x%08X  EDI: 0x%08X\n", ctx->Esi, ctx->Edi);
    printf("EBP: 0x%08X  ESP: 0x%08X\n", ctx->Ebp, ctx->Esp);
    printf("EIP: 0x%08X\n", ctx->Eip);
#endif
}

// Get breakpoint info for a specific address
Breakpoint::BPInfo* Breakpoint::GetBreakpointInfo(DWORD_PTR address) {
    for (int i = 0; i < 4; i++) {
        if (s_breakpoints[i].enabled && s_breakpoints[i].address == address) {
            return &s_breakpoints[i];
        }
    }
    return nullptr;
}