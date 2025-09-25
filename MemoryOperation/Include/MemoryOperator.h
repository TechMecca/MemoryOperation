#pragma once
#include "MemoryOperation.h"
#include "Patch.h"
#include "WinDetour.h"
#include <map>
#include <memory>
#include <string>

class MemoryOperator
{
public:
    static Patch* CreatePatch(const std::string& name, uintptr_t address, const std::vector<byte>& bytes);
    static WinDetour* CreateDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr);

    template<typename T>
    static WinDetour* CreateDetour(const std::string& name, uintptr_t target_addr, T&& detour_func)
    {
        // Store the detour function in a persistent object
        static auto persistent_func = std::forward<T>(detour_func);
        uintptr_t detour_addr = reinterpret_cast<uintptr_t>(+[]() {
            persistent_func();
            });

        return CreateDetour(name, target_addr, detour_addr);
    }

    static Patch*     FindPatch(const std::string& name);
    static WinDetour* FindDetour(const std::string& name);

private:
    static MemoryOperator& GetInstance();
    std::map<std::string, std::unique_ptr<MemoryOperation>> operations;
};