#pragma once
#include "MemoryOperation.h"
#include "Patch.h"
#include "WinDetour.h"
#include <map>
#include <memory>
#include <string>
#include <ranges>
#include <iostream>
#include <algorithm>



class MemoryOperator
{
public:

	static std::map<std::string, std::shared_ptr<MemoryOperation>>& GetOperations() { return operations; }

    static Patch*     CreatePatch(const std::string& name, uintptr_t address, const std::vector<byte>& bytes);
    static WinDetour* CreateDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr, bool Override);

    static Patch*     FindPatch(const std::string& name);
    static WinDetour* FindDetour(const std::string& name);
	static BOOL       DisposeAll(bool SaveActive, const std::vector<std::string>& ignoreList);
    static BOOL       ApplyAll(bool useSavedActive);

	static bool       IsLocationModified(const uintptr_t address, const size_t length, std::map<std::string, std::shared_ptr<MemoryOperation>>& ModifiedMemory);
    static bool       EraseAll();

    static bool DEBUG;

private:
    static std::map<std::string, std::shared_ptr<MemoryOperation>> operations;
    static std::map<std::string, std::shared_ptr<MemoryOperation>> Savedoperations;

};