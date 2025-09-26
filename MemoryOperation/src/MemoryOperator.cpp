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

//WinDetour* MemoryOperator::CreateDetour(const std::string& name, PVOID* targetAddress, PVOID detourFunction)
//{
//    auto& instance = GetInstance();
//    auto& ops = instance.operations;
//
//    if (ops.find(name) != ops.end()) {
//        std::cerr << "CreateDetour: name '" << name << "' already exists\n";
//        return nullptr;
//    }
//    if (!targetAddress || !*targetAddress || !detourFunction) {
//        std::cerr << "CreateDetour: null targetAddress/*targetAddress or detourFunction for '" << name << "'\n";
//        return nullptr;
//    }
//
//    try {
//        auto detour = std::make_shared<WinDetour>(targetAddress, detourFunction);
//        WinDetour* ptr = detour.get();
//        ops.emplace(name, std::move(detour));
//        return ptr;
//    }
//    catch (const std::exception& e) {
//        std::cerr << "CreateDetour: exception for '" << name << "': " << e.what() << "\n";
//        return nullptr;
//    }
//}

WinDetour* MemoryOperator::CreateDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr)
{
    auto& instance = GetInstance();
    auto& ops = instance.operations;

    if (ops.find(name) != ops.end()) {
        std::cerr << "CreateDetour(raw): name '" << name << "' already exists\n";
        return nullptr;
    }
    if (!target_addr || !detour_addr) {
        std::cerr << "CreateDetour(raw): null target/detour for '" << name << "'\n";
        return nullptr;
    }

    try {
        auto detour = std::make_shared<WinDetour>((PVOID*)target_addr, (PVOID)detour_addr); 
        WinDetour* ptr = detour.get();
        ops.emplace(name, std::move(detour));
        return ptr;
    }
    catch (const std::exception& e) {
        std::cerr << "CreateDetour(raw): exception for '" << name << "': " << e.what() << "\n";
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