#pragma once
#include <windows.h>
#include <detours.h>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <iostream> // for minimal logging (avoid in hooks)

#include "MemoryOperation.h"

// Minimal, safe wrapper around Microsoft Detours.
class WinDetour : public MemoryOperation
{
public:
    // Detours-style: pass &Real_Function (address of a function-pointer variable) and your hook.
    WinDetour(PVOID* targetAddressRef, PVOID detourFunction);

    // Raw addresses: we create per-instance storage that Detours can rewrite.
    WinDetour(uintptr_t targetAddress, uintptr_t detourFunction);

    ~WinDetour();

    bool   Apply()   override;  // Attach
    bool   Restore() override;  // Detach
    size_t GetLength() const override { return 0; } // Not meaningful for Detours

    template<typename T>
    T GetTrampoline() const { // cached after Apply()
        return reinterpret_cast<T>(trampoline_addr);
    }

    template<typename T>
    T GetOriginal() const {  // always reflects current Detours state
        return reinterpret_cast<T>(target_ptr ? *target_ptr : nullptr);
    }

    bool IsApplied() const { return is_modified; }

    // Tiny thread-local reentrancy guard; use inside your hook.
    struct ReentryGuard {
        static thread_local bool tls_in;
        bool entered_ok;
        ReentryGuard() : entered_ok(!tls_in) { tls_in = true; }
        ~ReentryGuard() { tls_in = false; }
    };

private:
    // Detours-managed pointers
    PVOID* target_ptr = nullptr; // -> function-pointer variable that Detours rewrites
    PVOID   detour_ptr = nullptr; // your hook

    // Storage used only when constructed from raw addresses
    PVOID   target_storage = nullptr;

    // Diagnostics
    uintptr_t target_addr_raw = 0;
    uintptr_t detour_addr = 0;
    uintptr_t trampoline_addr = 0;

private:
    void InitializeFromAddresses(uintptr_t targetAddress, uintptr_t detourFunction);

    static bool DetoursOK(LONG rc, const char* where);
    static void LogDetoursError(const char* where, LONG rc);
};
