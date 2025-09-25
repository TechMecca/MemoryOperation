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

    // Create, auto-add to manager, and return direct pointer
    static Patch* CreatePatch(const std::string& name, uintptr_t address, const std::vector<byte>& bytes);
    static WinDetour* CreateDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr);

    // Template version for detours with callables
    template<typename T>
    static WinDetour* CreateDetour(const std::string& name, uintptr_t target_addr, T&& detour_func);

    // Management functions
    bool RemoveOperation(const std::string& name);
    void RemoveAllOperations();

    // Find operations - return direct pointers
    MemoryOperation* FindOperation(const std::string& name);
    Patch* FindPatch(const std::string& name);
    WinDetour* FindDetour(const std::string& name);

    // Batch operations
    bool ApplyAll();
    bool RestoreAll();

    // Management
    size_t GetOperationCount() const;
    size_t GetAppliedCount() const;
    void PrintAllOperations() const;
    bool Exists(const std::string& name) const;

private:
    MemoryOperator() = default;
    ~MemoryOperator() = default;

    std::map<std::string, std::unique_ptr<MemoryOperation>> operations;
};

// Template implementation
template<typename T>
WinDetour* MemoryOperator::CreateDetour(const std::string& name, uintptr_t target_addr, T&& detour_func)
{
    try {
        auto detour = std::make_unique<WinDetour>(target_addr, std::forward<T>(detour_func));
        WinDetour* ptr = detour.get();
        GetInstance().operations[name] = std::move(detour);
        return ptr;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create detour '" << name << "': " << e.what() << std::endl;
        return nullptr;
    }
}