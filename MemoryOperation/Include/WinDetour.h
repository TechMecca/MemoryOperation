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
    WinDetour(uintptr_t targetAddress, uintptr_t funcAddress);

    ~WinDetour();

    bool   Apply()   override;  // Attach
    bool   Restore() override;  // Detach
    size_t GetLength() const override { return 0; } // Not meaningful for Detours

 
    bool IsApplied() const { return is_modified; }

  

private:
    uintptr_t   targetAddress;
    uintptr_t   HookAddress;
    PVOID       targetStorage;


};
