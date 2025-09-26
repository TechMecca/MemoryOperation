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
    static Patch*     CreatePatch(const std::string& name, uintptr_t address, const std::vector<byte>& bytes);
    //static WinDetour* CreateDetour(const std::string& name, PVOID* targetAddress, PVOID detourFunction);
    static WinDetour* CreateDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr);

    static Patch*     FindPatch(const std::string& name);
    static WinDetour* FindDetour(const std::string& name);

    static bool DEBUG;

private:
    static MemoryOperator& GetInstance();
    std::map<std::string, std::shared_ptr<MemoryOperation>> operations;
};