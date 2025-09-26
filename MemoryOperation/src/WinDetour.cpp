#include <windows.h>
#include <cstdint>
#include <stdexcept>
#include <iostream>

#include "WinDetour.h"
#include "MemoryOperation.h" // for is_modified, etc.




//
WinDetour::WinDetour(PVOID* targetAddressRef, PVOID detourFunction)
{
    if (!targetAddressRef || !*targetAddressRef || !detourFunction) {
        throw std::invalid_argument("WinDetour: null targetAddressRef/*targetAddressRef or detourFunction");
    }

    // Keep the caller's variable — Detours will rewrite *this variable*.
    targetAddress = targetAddressRef;
    HookAddress = detourFunction;

    // targetStorage is not used for the attach in this path, but we mirror the
    // current value (the real entry) for logging/debugging if you need it.
    targetStorage = *targetAddressRef; // e.g., 0x0819210

    std::cout << "Detour prepared (ref): target=0x" << std::hex
        << reinterpret_cast<uintptr_t>(*targetAddress)
        << " detour=0x" << reinterpret_cast<uintptr_t>(HookAddress)
        << " (targetStorage mirror=0x" << reinterpret_cast<uintptr_t>(targetStorage)
        << ")" << std::dec << "\n";
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

    rc = DetourAttach((PVOID*)targetAddress, (PVOID)HookAddress);
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

    rc = DetourDetach(targetAddress, HookAddress);
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
