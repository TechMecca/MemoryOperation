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

    // Add operations
    bool AddPatch(const std::string& name, uintptr_t address, const std::vector<byte>& bytes);
    bool AddDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr);

    // Template version for detours with callables
    template<typename T>
    bool AddDetour(const std::string& name, uintptr_t target_addr, T&& detour_func);

    // Remove operations
    bool RemoveOperation(const std::string& name);
    void RemoveAllOperations();

    // Find operations
    std::shared_ptr<MemoryOperation> FindOperation(const std::string& name);
    std::shared_ptr<Patch> FindPatch(const std::string& name);
    std::shared_ptr<WinDetour> FindDetour(const std::string& name);

    // Apply/Restore operations
    bool ApplyOperation(const std::string& name);
    bool RestoreOperation(const std::string& name);
    bool ApplyAll();
    bool RestoreAll();

    // Management
    size_t GetOperationCount() const;
    size_t GetAppliedCount() const;
    void PrintAllOperations() const;
    bool Exists(const std::string& name) const;

    // Get all operations
    const std::map<std::string, std::shared_ptr<MemoryOperation>>& GetAllOperations() const {
        return operations;
    }

private:
    MemoryOperator() = default;
    ~MemoryOperator() = default;

    std::map<std::string, std::shared_ptr<MemoryOperation>> operations;
};