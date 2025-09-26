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

    static MemoryOperator& GetInstance();
	static std::map<std::string, std::shared_ptr<MemoryOperation>>& GetOperations() { return GetInstance().operations; }

    static Patch*     CreatePatch(const std::string& name, uintptr_t address, const std::vector<byte>& bytes);
    static WinDetour* CreateDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr, bool Override);

    static Patch*     FindPatch(const std::string& name);
    static WinDetour* FindDetour(const std::string& name);

    static bool DEBUG;

private:
    std::map<std::string, std::shared_ptr<MemoryOperation>> operations;
};