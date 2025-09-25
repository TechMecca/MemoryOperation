#pragma once
#include "MemoryOperation.h"
#include "detours.h"
#include <functional>

class WinDetour : public MemoryOperation
{
public:
    // Constructor for detouring a function to another function
    WinDetour(uintptr_t target_addr, uintptr_t detour_addr);

    // Constructor for detouring with std::function (for lambdas/member functions)
    template<typename T>
    WinDetour(uintptr_t target_addr, T&& detour_func);

    ~WinDetour();

    bool Apply() override;
    bool Restore() override;
    size_t GetLength() const override;

    // Get the trampoline function to call the original
    template<typename T>
    T GetTrampoline() const {
        return reinterpret_cast<T>(trampoline_addr);
    }

    bool IsApplied() const { return is_modified; }

private:
    uintptr_t detour_addr = 0;
    uintptr_t trampoline_addr = 0;
    void* target_ptr = nullptr;
    void* detour_ptr = nullptr;

    void InitializeDetour(uintptr_t target_addr, uintptr_t detour_addr);
    void BackupOriginalBytes();
};