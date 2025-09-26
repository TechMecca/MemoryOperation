#include <windows.h>
#include <cstdint>
#include <stdexcept>
#include <iostream>

#include "WinDetour.h"
#include "MemoryOperation.h" // for is_modified, etc.

WinDetour::WinDetour(uintptr_t targetAddress, uintptr_t detourFunction)
{
    if (!targetAddress || !detourFunction) {
        throw std::invalid_argument("WinDetour: null targetAddress or detourFunction");
    }

    this->targetAddress = targetAddress;
    this->HookAddress = detourFunction;

    // Pre-attach: Detours requires a writable variable whose value it will replace
    // with the trampoline. We keep that variable as a member: targetStorage.
    targetStorage = reinterpret_cast<PVOID>(targetAddress);

    std::cout << "Detour prepared (raw): target=0x" << std::hex << this->targetAddress
        << " detour=0x" << this->HookAddress << std::dec << "\n";
}

WinDetour::~WinDetour()
{
    // Best-effort restore; don't throw from a destructor.
    if (is_modified) {
        try { Restore(); }
        catch (...) {}
    }
}

bool WinDetour::Apply()
{
    if (is_modified) {
        std::cout << "Detour already applied\n";
        return true;
    }

    if (!targetStorage || !HookAddress) {
        std::cerr << "Apply: invalid state (targetStorage=" << targetStorage
            << ", HookAddress=0x" << std::hex << HookAddress << std::dec << ")\n";
        return false;
    }

    LONG rc = DetourTransactionBegin();
    if (rc != NO_ERROR) {
        std::cerr << "DetourTransactionBegin failed (rc=" << rc << ")\n";
        return false;
    }

    rc = DetourUpdateThread(GetCurrentThread());
    if (rc != NO_ERROR) {
        std::cerr << "DetourUpdateThread failed (rc=" << rc << ")\n";
        DetourTransactionAbort();
        return false;
    }

    PVOID detourPtr = reinterpret_cast<PVOID>(HookAddress);
    rc = DetourAttach(reinterpret_cast<PVOID*>(&targetStorage), detourPtr);
    if (rc != NO_ERROR) {
        std::cerr << "DetourAttach failed (rc=" << rc << ")\n";
        DetourTransactionAbort();
        return false;
    }

    rc = DetourTransactionCommit();
    if (rc != NO_ERROR) {
        std::cerr << "DetourTransactionCommit failed (rc=" << rc << ")\n";
        DetourTransactionAbort();
        return false;
    }

    // After commit, targetStorage now points to the trampoline (the original function).
    is_modified = true;

    std::cout << "Detour applied: target=0x" << std::hex << this->targetAddress
        << " trampoline=0x" << reinterpret_cast<uintptr_t>(targetStorage)
        << std::dec << "\n";
    return true;
}

bool WinDetour::Restore()
{
    if (!is_modified) {
        std::cout << "Detour not applied; nothing to restore\n";
        return true;
    }

    if (!targetStorage || !HookAddress) {
        std::cerr << "Restore: invalid state\n";
        return false;
    }

    LONG rc = DetourTransactionBegin();
    if (rc != NO_ERROR) {
        std::cerr << "DetourTransactionBegin failed (rc=" << rc << ")\n";
        return false;
    }

    rc = DetourUpdateThread(GetCurrentThread());
    if (rc != NO_ERROR) {
        std::cerr << "DetourUpdateThread failed (rc=" << rc << ")\n";
        DetourTransactionAbort();
        return false;
    }

    PVOID detourPtr = reinterpret_cast<PVOID>(HookAddress);
    rc = DetourDetach(reinterpret_cast<PVOID*>(&targetStorage), detourPtr);
    if (rc != NO_ERROR) {
        std::cerr << "DetourDetach failed (rc=" << rc << ")\n";
        DetourTransactionAbort();
        return false;
    }

    rc = DetourTransactionCommit();
    if (rc != NO_ERROR) {
        std::cerr << "DetourTransactionCommit failed (rc=" << rc << ")\n";
        DetourTransactionAbort();
        return false;
    }

    is_modified = false;
    std::cout << "Detour restored: target=0x" << std::hex << this->targetAddress << std::dec << "\n";
    return true;
}
