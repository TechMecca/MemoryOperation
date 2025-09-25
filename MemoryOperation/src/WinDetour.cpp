#include "WinDetour.h"
#include <cstring>

thread_local bool WinDetour::ReentryGuard::tls_in = false;

void WinDetour::LogDetoursError(const char* where, LONG rc) {
    std::cerr << where << " failed (rc=" << rc << ")\n";
}

bool WinDetour::DetoursOK(LONG rc, const char* where) {
    if (rc == NO_ERROR) return true;
    LogDetoursError(where, rc);
    return false;
}

// ---------------- Constructors ----------------

WinDetour::WinDetour(PVOID* targetAddressRef, PVOID detourFunction)
{
    if (!targetAddressRef || !*targetAddressRef || !detourFunction) {
        throw std::invalid_argument("WinDetour: null targetAddressRef/*targetAddressRef or detourFunction");
    }

    target_ptr = targetAddressRef;                 // Detours will rewrite *target_ptr
    detour_ptr = detourFunction;
    target_addr_raw = reinterpret_cast<uintptr_t>(*targetAddressRef);
    detour_addr = reinterpret_cast<uintptr_t>(detourFunction);

    address = target_addr_raw; // from MemoryOperation (for consistency)

    std::cout << "Detour prepared (ref): target=0x" << std::hex << target_addr_raw
        << " detour=0x" << detour_addr << std::dec << "\n";
}

WinDetour::WinDetour(uintptr_t targetAddress, uintptr_t detourFunction)
{
    InitializeFromAddresses(targetAddress, detourFunction);
}

void WinDetour::InitializeFromAddresses(uintptr_t targetAddress, uintptr_t detourFunction)
{
    if (!targetAddress || !detourFunction) {
        throw std::invalid_argument("WinDetour: null target/detour address");
    }

    // Per-instance storage Detours can rewrite safely
    target_storage = reinterpret_cast<PVOID>(targetAddress);
    target_ptr = &target_storage;
    detour_ptr = reinterpret_cast<PVOID>(detourFunction);

    target_addr_raw = targetAddress;
    detour_addr = detourFunction;
    address = target_addr_raw;

    std::cout << "Detour prepared (raw): target=0x" << std::hex << target_addr_raw
        << " detour=0x" << detour_addr << std::dec << "\n";
}

// ---------------- Destructor ----------------

WinDetour::~WinDetour()
{
    if (is_modified) {
        // best-effort; do not throw from dtor
        Restore();
    }
}

// ---------------- Apply / Restore ----------------

bool WinDetour::Apply()
{
    if (is_modified) {
        std::cout << "Detour already applied\n";
        return true;
    }

    if (!target_ptr || !*target_ptr || !detour_ptr) {
        std::cerr << "Apply: invalid pointers (target_ptr=" << target_ptr
            << ", *target_ptr=" << (target_ptr ? *target_ptr : nullptr)
            << ", detour_ptr=" << detour_ptr << ")\n";
        return false;
    }

    if (!DetoursOK(DetourTransactionBegin(), "DetourTransactionBegin")) return false;

    // IMPORTANT: only update the current thread *unless* you know/want to quiesce all threads.
    // Updating every thread incorrectly can deadlock if you grab foreign threads that hold locks.
    if (!DetoursOK(DetourUpdateThread(GetCurrentThread()), "DetourUpdateThread")) {
        DetourTransactionAbort(); return false;
    }

    if (!DetoursOK(DetourAttach(reinterpret_cast<PVOID*>(target_ptr), detour_ptr), "DetourAttach")) {
        DetourTransactionAbort(); return false;
    }

    if (!DetoursOK(DetourTransactionCommit(), "DetourTransactionCommit")) {
        DetourTransactionAbort(); return false;
    }

    // After commit, *target_ptr points to the trampoline (original)
    trampoline_addr = reinterpret_cast<uintptr_t>(*target_ptr);
    is_modified = true;

    std::cout << "Detour applied: target=0x" << std::hex << target_addr_raw
        << " trampoline=0x" << trampoline_addr << std::dec << "\n";
    return true;
}

bool WinDetour::Restore()
{
    if (!is_modified) {
        std::cout << "Detour not applied; nothing to restore\n";
        return true;
    }
    if (!target_ptr || !detour_ptr) {
        std::cerr << "Restore: invalid pointers\n";
        return false;
    }

    if (!DetoursOK(DetourTransactionBegin(), "DetourTransactionBegin")) return false;
    if (!DetoursOK(DetourUpdateThread(GetCurrentThread()), "DetourUpdateThread")) {
        DetourTransactionAbort(); return false;
    }

    if (!DetoursOK(DetourDetach(reinterpret_cast<PVOID*>(target_ptr), detour_ptr), "DetourDetach")) {
        DetourTransactionAbort(); return false;
    }

    if (!DetoursOK(DetourTransactionCommit(), "DetourTransactionCommit")) {
        DetourTransactionAbort(); return false;
    }

    is_modified = false;
    std::cout << "Detour restored: target=0x" << std::hex << target_addr_raw << std::dec << "\n";
    return true;
}
