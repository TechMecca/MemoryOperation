#pragma once
#include "MemoryOperation.h"
#include "detours.h"
#include <functional>

class WinDetour : public MemoryOperation
{
public:
    // Two constructors for flexibility
    WinDetour(PVOID* targetAddress, PVOID detourFunction);  // For Detours-style (PVOID*)
    WinDetour(uintptr_t targetAddress, uintptr_t detourFunction);  // For direct addresses

    ~WinDetour();

    bool Apply() override;
    bool Restore() override;
    size_t GetLength() const override;

    template<typename T>
    T GetTrampoline() const {
        return reinterpret_cast<T>(trampoline_addr);
    }

    bool IsApplied() const { return is_modified; }

private:
    uintptr_t detour_addr = 0;
    uintptr_t trampoline_addr = 0;
    PVOID* target_ptr = nullptr;  // Changed to PVOID*
    PVOID detour_ptr = nullptr;

    void BackupOriginalBytes();
    void InitializeFromAddresses(uintptr_t targetAddress, uintptr_t detourFunction);
};