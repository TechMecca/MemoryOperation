#include "WinDetour.h"
#include <algorithm>
#include <cstring>
#include <iostream>

static void LogErr(const char* where, LONG rc) {
    std::cerr << where << " failed (rc=" << rc << ")\n";
}

bool WinDetour::DetoursOK(LONG rc, const char* where) {
    if (rc == NO_ERROR) return true;
    LogErr(where, rc);
    return false;
}

// ---------- Constructors ----------

WinDetour::WinDetour(PVOID* targetAddress, PVOID detourFunction)
{
    if (!targetAddress || !*targetAddress || !detourFunction) {
        throw std::invalid_argument("WinDetour: targetAddress/*targetAddress or detourFunction is null");
    }

    target_ptr = targetAddress;               // Detours will rewrite *target_ptr
    detour_ptr = detourFunction;
    target_addr_raw = reinterpret_cast<uintptr_t>(*targetAddress);
    detour_addr = reinterpret_cast<uintptr_t>(detourFunction);

    // Keep base-class fields consistent (if used elsewhere)
    address = target_addr_raw;

    // Optional; see comment inside
    BackupOriginalBytes();

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
        throw std::invalid_argument("WinDetour: target/detour address is null");
    }

    // Per-instance storage that Detours can safely rewrite
    target_storage = reinterpret_cast<PVOID>(targetAddress);
    target_ptr = &target_storage;
    detour_ptr = reinterpret_cast<PVOID>(detourFunction);

    target_addr_raw = targetAddress;
    detour_addr = detourFunction;
    address = target_addr_raw;

    // Optional; see comment inside
    BackupOriginalBytes();

    std::cout << "Detour prepared (raw): target=0x" << std::hex << target_addr_raw
        << " detour=0x" << detour_addr << std::dec << "\n";
}

// ---------- Back up (optional) ----------

void WinDetour::BackupOriginalBytes()
{
    // Detours does safe patching; a backup is not required to detach.
    // If your MemoryOperation tooling expects it for diagnostics, we copy up to 32 bytes
    // but only within the committed region to avoid faults.
    original_bytes.clear();

    MEMORY_BASIC_INFORMATION mbi{};
    SIZE_T got = VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi));
    if (got != sizeof(mbi)) return;
    if (mbi.State != MEM_COMMIT)   return;

    uintptr_t start = address;
    uintptr_t region_end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + static_cast<uintptr_t>(mbi.RegionSize);
    if (region_end <= start) return;

    const size_t max_copy = 32u;
    size_t avail = static_cast<size_t>(region_end - start);
    size_t to_copy = (avail < max_copy) ? avail : max_copy; // **Initialized here** (fix for C2737)

    DWORD old_protection = 0;
    if (SetMemoryProtection(address, to_copy, PAGE_EXECUTE_READWRITE, &old_protection)) {
        original_bytes.resize(to_copy);
        std::memcpy(original_bytes.data(), reinterpret_cast<const void*>(address), to_copy);

        DWORD tmp = 0;
        SetMemoryProtection(address, to_copy, old_protection, &tmp);
    }
}

// ---------- Destructor ----------

WinDetour::~WinDetour()
{
    if (is_modified) {
        // best-effort; don’t throw from dtor
        Restore();
    }
}

// ---------- Apply / Restore ----------

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
    if (!DetoursOK(DetourUpdateThread(GetCurrentThread()), "DetourUpdateThread")) { DetourTransactionAbort(); return false; }

    if (!DetoursOK(DetourAttach(reinterpret_cast<PVOID*>(target_ptr), detour_ptr), "DetourAttach")) {
        DetourTransactionAbort(); return false;
    }

    if (!DetoursOK(DetourTransactionCommit(), "DetourTransactionCommit")) {
        DetourTransactionAbort(); return false;
    }

    // After successful commit, *target_ptr points to the trampoline (original)
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
    if (!DetoursOK(DetourUpdateThread(GetCurrentThread()), "DetourUpdateThread")) { DetourTransactionAbort(); return false; }

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

// ---------- Misc ----------

size_t WinDetour::GetLength() const
{
    // Detours doesn’t use an inline patch length here; return whatever we backed up
    return original_bytes.size();
}
