#include "WinDetour.h"
#include <stdexcept>
#include <iostream>

WinDetour::WinDetour(PVOID* targetAddress, PVOID detourFunction)
{
    if (!targetAddress || !detourFunction) {
        throw std::invalid_argument("Target address and detour function cannot be null");
    }

    // Store the addresses
    this->target_ptr = targetAddress;
    this->detour_ptr = detourFunction;
    this->address = reinterpret_cast<uintptr_t>(*targetAddress);



    // Backup original bytes
    BackupOriginalBytes();

    std::cout << "Detour prepared (not applied):\n";
    std::cout << "  Target: 0x" << std::hex << address << "\n";
    std::cout << "  Detour: 0x" << std::hex << detour_addr << "\n";
}


void WinDetour::BackupOriginalBytes()
{
    // Backup the original bytes that Detours will overwrite
    // Detours typically overwrites 5-32 bytes depending on the architecture
    const size_t typical_detour_size = 32; // Safe size for x86/x64
    original_bytes.resize(typical_detour_size);

    DWORD old_protection;
    if (SetMemoryProtection(address, typical_detour_size, PAGE_EXECUTE_READWRITE, &old_protection)) {
        memcpy(original_bytes.data(), reinterpret_cast<void*>(address), typical_detour_size);
        DWORD temp;
        SetMemoryProtection(address, typical_detour_size, old_protection, &temp);
    }
}

WinDetour::~WinDetour()
{
    if (is_modified) {
        Restore();
    }
}

bool WinDetour::Apply()
{
    if (is_modified)
    {
        std::cout << "Detour already applied\n";
        return true; // Already applied
    }

    if (address == 0 || detour_addr == 0) {
        return false;
    }

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (DetourAttach(&target_ptr, detour_ptr) != NO_ERROR) {
        DetourTransactionAbort();
        std::cout << "Failed to attach detour\n";
        return false;
    }

    if (DetourTransactionCommit() == NO_ERROR) {
        // Store the trampoline address (only on first apply)
        if (trampoline_addr == 0) {
            trampoline_addr = reinterpret_cast<uintptr_t>(target_ptr);
        }
        is_modified = true;

        std::cout << "Detour applied successfully to 0x" << std::hex << address << "\n";
        std::cout << "Trampoline at: 0x" << std::hex << trampoline_addr << std::endl;
        return true;
    }

    DetourTransactionAbort();
    std::cout << "Failed to commit detour transaction\n";
    return false;
}

bool WinDetour::Restore()
{
    if (!is_modified) {
        std::cout << "Detour not applied, nothing to restore\n";
        return true; // Not applied, so nothing to restore
    }

    if (address == 0)
    {
        return false;
    }

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    if (DetourDetach(&target_ptr, detour_ptr) != NO_ERROR) {
        DetourTransactionAbort();
        std::cout << "Failed to detach detour\n";
        return false;
    }

    if (DetourTransactionCommit() == NO_ERROR) {
        is_modified = false;
        // DO NOT reset trampoline_addr here - we need it for future applies!
        // trampoline_addr = 0; // ? This would break re-application

        std::cout << "Detour restored successfully at 0x" << std::hex << address << std::endl;
        std::cout << "Trampoline preserved at: 0x" << std::hex << trampoline_addr << std::endl;
        return true;
    }

    DetourTransactionAbort();
    std::cout << "Failed to commit detour restore transaction\n";
    return false;
}

size_t WinDetour::GetLength() const
{
    return original_bytes.size();
}