#include "Breakpoint.h"
#include <stdio.h>

// Static members initialization
Breakpoint::BPInfo Breakpoint::s_breakpoints[4];
bool Breakpoint::s_handlerInstalled = false;

void Breakpoint::InstallHandler() {
    if (!s_handlerInstalled) {
        AddVectoredExceptionHandler(1, ExceptionHandler);
        s_handlerInstalled = true;
    }
}

int Breakpoint::FindFreeDebugRegister() {
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    if (!GetThreadContext(GetCurrentThread(), &ctx)) {
        return -1;
    }

    // Check which debug registers are available
    if (!(ctx.Dr7 & 1)) return 0;   // DR0 free
    if (!(ctx.Dr7 & 4)) return 1;   // DR1 free  
    if (!(ctx.Dr7 & 16)) return 2;  // DR2 free
    if (!(ctx.Dr7 & 64)) return 3;  // DR3 free

    return -1; // No registers available
}

void Breakpoint::UpdateDebugRegisters() {
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    if (!GetThreadContext(GetCurrentThread(), &ctx)) {
        return;
    }

    // Clear all debug registers
    ctx.Dr0 = 0;
    ctx.Dr1 = 0;
    ctx.Dr2 = 0;
    ctx.Dr3 = 0;
    ctx.Dr7 = 0;

    // Re-set all active breakpoints
    for (int i = 0; i < 4; i++) {
        if (s_breakpoints[i].enabled && !s_breakpoints[i].singleStepping) {
            switch (i) {
            case 0: ctx.Dr0 = s_breakpoints[i].address; break;
            case 1: ctx.Dr1 = s_breakpoints[i].address; break;
            case 2: ctx.Dr2 = s_breakpoints[i].address; break;
            case 3: ctx.Dr3 = s_breakpoints[i].address; break;
            }
            ctx.Dr7 |= (1 << (2 * i)); // Enable breakpoint
        }
    }

    SetThreadContext(GetCurrentThread(), &ctx);
}

void Breakpoint::DisableBreakpoint(int drIndex) {
    if (drIndex < 0 || drIndex > 3) return;

    s_breakpoints[drIndex].singleStepping = true;
    UpdateDebugRegisters();
}

void Breakpoint::EnableBreakpoint(int drIndex) {
    if (drIndex < 0 || drIndex > 3) return;

    s_breakpoints[drIndex].singleStepping = false;
    UpdateDebugRegisters();
}

void Breakpoint::SingleStepInstruction(HANDLE hThread) {
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_CONTROL;

    if (GetThreadContext(hThread, &ctx)) {
        ctx.EFlags |= 0x100; // Set trap flag for single step
        SetThreadContext(hThread, &ctx);
    }
}

LONG WINAPI Breakpoint::ExceptionHandler(PEXCEPTION_POINTERS ex) {
    if (ex->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP) {
        DWORD_PTR hitAddress = (DWORD_PTR)ex->ExceptionRecord->ExceptionAddress;

        // Check if this single-step was from our breakpoint
        for (int i = 0; i < 4; i++) {
            if (s_breakpoints[i].singleStepping) {
                // Re-enable the breakpoint now that we've single-stepped
                EnableBreakpoint(i);
                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }

        // Handle regular breakpoint hits
        for (int i = 0; i < 4; i++) {
            if (s_breakpoints[i].enabled && s_breakpoints[i].address == hitAddress && !s_breakpoints[i].singleStepping) {
                if (s_breakpoints[i].callback) {
                    s_breakpoints[i].callback(ex->ContextRecord);
                }

                // Disable breakpoint temporarily and single-step over the instruction
                DisableBreakpoint(i);
                SingleStepInstruction(GetCurrentThread());

                return EXCEPTION_CONTINUE_EXECUTION;
            }
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

bool Breakpoint::Set(DWORD_PTR address, Callback callback) {
    if (!address || !callback) {
        return false;
    }

    InstallHandler();

    // Remove existing breakpoint at this address
    Remove(address);

    int drIndex = FindFreeDebugRegister();
    if (drIndex == -1) {
        printf("[Breakpoint] No debug registers available! Maximum of 4 breakpoints supported.\n");
        return false;
    }

    // Store breakpoint info
    s_breakpoints[drIndex] = { address, callback, drIndex, true, false };

    // Update hardware registers
    UpdateDebugRegisters();

    printf("[Breakpoint] Set at 0x%08X (DR%d)\n", address, drIndex);
    return true;
}

bool Breakpoint::Remove(DWORD_PTR address) {
    for (int i = 0; i < 4; i++) {
        if (s_breakpoints[i].enabled && s_breakpoints[i].address == address) {
            s_breakpoints[i].enabled = false;
            s_breakpoints[i].callback = nullptr;
            s_breakpoints[i].singleStepping = false;

            UpdateDebugRegisters();

            printf("[Breakpoint] Removed at 0x%08X\n", address);
            return true;
        }
    }
    return false;
}

void Breakpoint::RemoveAll() {
    for (int i = 0; i < 4; i++) {
        s_breakpoints[i].enabled = false;
        s_breakpoints[i].callback = nullptr;
        s_breakpoints[i].singleStepping = false;
    }

    UpdateDebugRegisters();
    printf("[Breakpoint] All breakpoints removed\n");
}

bool Breakpoint::IsSet(DWORD_PTR address) {
    for (int i = 0; i < 4; i++) {
        if (s_breakpoints[i].enabled && s_breakpoints[i].address == address) {
            return true;
        }
    }
    return false;
}

int Breakpoint::GetCount() {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (s_breakpoints[i].enabled) {
            count++;
        }
    }
    return count;
}

void Breakpoint::PrintRegisters(PCONTEXT ctx) {
    if (!ctx) return;

    printf("=== CPU Registers ===\n");
    printf("EAX: 0x%08X\n", ctx->Eax);
    printf("EBX: 0x%08X\n", ctx->Ebx);
    printf("ECX: 0x%08X\n", ctx->Ecx);
    printf("EDX: 0x%08X\n", ctx->Edx);
    printf("ESI: 0x%08X\n", ctx->Esi);
    printf("EDI: 0x%08X\n", ctx->Edi);
    printf("EBP: 0x%08X\n", ctx->Ebp);
    printf("ESP: 0x%08X\n", ctx->Esp);
    printf("EIP: 0x%08X\n", ctx->Eip);
    printf("EFLAGS: 0x%08X\n", ctx->EFlags);

    // Stack contents
    printf("\n=== Stack (top 8 values) ===\n");
    DWORD* esp = (DWORD*)ctx->Esp;
    for (int i = 0; i < 8; i++) {
        DWORD value = 0;
        if (esp && !IsBadReadPtr(esp, sizeof(DWORD))) {
            value = *esp;
        }
        printf("ESP+%d: 0x%08X\n", i * 4, value);
        esp++;
    }
    printf("=============================\n\n");
}

DWORD Breakpoint::GetRegisterValue(PCONTEXT ctx, const char* regName) {
    if (!ctx || !regName) return 0;

    if (_stricmp(regName, "eax") == 0) return ctx->Eax;
    if (_stricmp(regName, "ebx") == 0) return ctx->Ebx;
    if (_stricmp(regName, "ecx") == 0) return ctx->Ecx;
    if (_stricmp(regName, "edx") == 0) return ctx->Edx;
    if (_stricmp(regName, "esi") == 0) return ctx->Esi;
    if (_stricmp(regName, "edi") == 0) return ctx->Edi;
    if (_stricmp(regName, "ebp") == 0) return ctx->Ebp;
    if (_stricmp(regName, "esp") == 0) return ctx->Esp;
    if (_stricmp(regName, "eip") == 0) return ctx->Eip;

    printf("[Breakpoint] Unknown register: %s\n", regName);
    return 0;
}