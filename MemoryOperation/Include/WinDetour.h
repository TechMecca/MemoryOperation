#pragma once
#include <windows.h>
#include <detours.h>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "MemoryOperation.h"

// Wrapper around Microsoft Detours that supports either a PVOID* (Detours-style)
// or raw addresses (we create per-instance storage Detours can rewrite).
class WinDetour : public MemoryOperation
{
public:
    // For Detours-style usage: pass the address of your function-pointer variable
    // (e.g., &Real_Foo) and the hook function pointer.
    WinDetour(PVOID* targetAddress, PVOID detourFunction);

    // For raw addresses: pass the real target address and hook address.
    WinDetour(uintptr_t targetAddress, uintptr_t detourFunction);

    ~WinDetour();

    bool   Apply()   override;   // attach
    bool   Restore() override;   // detach
    size_t GetLength() const override; // retained for your base class; not meaningful for Detours

    template<typename T>
    T GetTrampoline() const { // cached trampoline after Apply()
        return reinterpret_cast<T>(trampoline_addr);
    }

    // Always returns the current "original" function pointer Detours has stored
    // in the rewritable variable (works even after re-attach).
    template<typename T>
    T GetOriginal() const {
        return reinterpret_cast<T>(target_ptr ? *target_ptr : nullptr);
    }

    bool IsApplied() const { return is_modified; }

private:
    // Detours-managed pointers
    PVOID* target_ptr = nullptr;   // points to a function-pointer variable that Detours rewrites
    PVOID   detour_ptr = nullptr;   // hook function

    // Per-instance storage used when constructed from raw addresses
    PVOID   target_storage = nullptr;

    // Informational / diagnostics
    uintptr_t detour_addr = 0;
    uintptr_t trampoline_addr = 0;  // cached after first successful attach
    uintptr_t target_addr_raw = 0;  // original target (for logs)

private:
    void InitializeFromAddresses(uintptr_t targetAddress, uintptr_t detourFunction);
    void BackupOriginalBytes(); // bounded & optional; safe even if left unused

    static bool DetoursOK(LONG rc, const char* where);
};
