#include "MemoryOperator.h"
#include <iostream>

MemoryOperator& MemoryOperator::GetInstance()
{
    static MemoryOperator instance;
    return instance;
}

Patch* MemoryOperator::CreatePatch(const std::string& name, uintptr_t address, const std::vector<byte>& bytes)
{
    auto& instance = GetInstance();

    if (instance.operations.find(name) != instance.operations.end()) {
        return nullptr; // Name already exists
    }

    try {
        auto patch = std::make_unique<Patch>(address, bytes);
        Patch* ptr = patch.get();
        instance.operations[name] = std::move(patch);
        return ptr;
    }
    catch (const std::exception&) {
        return nullptr;
    }
}

WinDetour* MemoryOperator::CreateDetour(const std::string& name, PVOID* targetAddress, PVOID detourFunction)
{
    auto& instance = GetInstance();

    if (instance.operations.find(name) != instance.operations.end()) {
        return nullptr; // Name already exists
    }

    try {
        auto detour = std::make_unique<WinDetour>(targetAddress, detourFunction);
        WinDetour* ptr = detour.get();
        instance.operations[name] = std::move(detour);
        return ptr;
    }
    catch (const std::exception&) {
        return nullptr;
    }
}
Patch* MemoryOperator::FindPatch(const std::string& name)
{
    auto& instance = GetInstance();
    auto it = instance.operations.find(name);
    if (it != instance.operations.end()) {
        return dynamic_cast<Patch*>(it->second.get());
    }
    return nullptr;
}

WinDetour* MemoryOperator::FindDetour(const std::string& name)
{
    auto& instance = GetInstance();
    auto it = instance.operations.find(name);
    if (it != instance.operations.end()) {
        return dynamic_cast<WinDetour*>(it->second.get());
    }
    return nullptr;
}