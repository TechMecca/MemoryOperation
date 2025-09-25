#include "MemoryOperator.h"
#include <iostream>
#include <typeinfo>

MemoryOperator& MemoryOperator::GetInstance()
{
    static MemoryOperator instance;
    return instance;
}

Patch* MemoryOperator::CreatePatch(const std::string& name, uintptr_t address, const std::vector<byte>& bytes)
{
    if (GetInstance().Exists(name)) {
        std::cout << "Operation '" << name << "' already exists\n";
        return nullptr;
    }

    if (address == 0 || bytes.empty()) {
        std::cout << "Invalid parameters for patch '" << name << "'\n";
        return nullptr;
    }

    try
    {
        auto patch = std::make_unique<Patch>(address, bytes);
        Patch* ptr = patch.get();
        GetInstance().operations[name] = std::move(patch);
        std::cout << "Patch '" << name << "' created successfully at 0x" << std::hex << address << std::dec << "\n";
        return ptr;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create patch '" << name << "': " << e.what() << std::endl;
        return nullptr;
    }
}

WinDetour* MemoryOperator::CreateDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr)
{
    if (GetInstance().Exists(name)) {
        std::cout << "Operation '" << name << "' already exists\n";
        return nullptr;
    }

    if (target_addr == 0 || detour_addr == 0) {
        std::cout << "Invalid parameters for detour '" << name << "'\n";
        return nullptr;
    }

    try {
        auto detour = std::make_unique<WinDetour>(target_addr, detour_addr);
        WinDetour* ptr = detour.get();
        GetInstance().operations[name] = std::move(detour);
        std::cout << "Detour '" << name << "' created successfully (target: 0x" << std::hex << target_addr
            << ", detour: 0x" << detour_addr << ")\n" << std::dec;
        return ptr;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create detour '" << name << "': " << e.what() << std::endl;
        return nullptr;
    }
}

bool MemoryOperator::RemoveOperation(const std::string& name)
{
    auto& instance = GetInstance();
    auto it = instance.operations.find(name);
    if (it != instance.operations.end()) {
        // Auto-restore if applied
        if (it->second->is_modified) {
            it->second->Restore();
        }
        instance.operations.erase(it);
        std::cout << "Operation '" << name << "' removed successfully\n";
        return true;
    }
    std::cout << "Operation '" << name << "' not found\n";
    return false;
}

void MemoryOperator::RemoveAllOperations()
{
    auto& instance = GetInstance();
    for (auto& [name, op] : instance.operations) {
        if (op->is_modified) {
            op->Restore();
        }
    }
    instance.operations.clear();
    std::cout << "All operations removed\n";
}

MemoryOperation* MemoryOperator::FindOperation(const std::string& name)
{
    auto& instance = GetInstance();
    auto it = instance.operations.find(name);
    if (it != instance.operations.end()) {
        return it->second.get();
    }
    return nullptr;
}

Patch* MemoryOperator::FindPatch(const std::string& name)
{
    MemoryOperation* op = FindOperation(name);
    if (op) {
        return dynamic_cast<Patch*>(op);
    }
    return nullptr;
}

WinDetour* MemoryOperator::FindDetour(const std::string& name)
{
    MemoryOperation* op = FindOperation(name);
    if (op) {
        return dynamic_cast<WinDetour*>(op);
    }
    return nullptr;
}

bool MemoryOperator::ApplyAll()
{
    auto& instance = GetInstance();
    bool all_success = true;

    for (auto& [name, op] : instance.operations) {
        if (!op->is_modified) {
            if (!op->Apply()) {
                std::cout << "Failed to apply operation '" << name << "'\n";
                all_success = false;
            }
            else {
                std::cout << "Applied operation '" << name << "'\n";
            }
        }
    }
    return all_success;
}

bool MemoryOperator::RestoreAll()
{
    auto& instance = GetInstance();
    bool all_success = true;

    for (auto& [name, op] : instance.operations) {
        if (op->is_modified) {
            if (!op->Restore()) {
                std::cout << "Failed to restore operation '" << name << "'\n";
                all_success = false;
            }
            else {
                std::cout << "Restored operation '" << name << "'\n";
            }
        }
    }
    return all_success;
}

size_t MemoryOperator::GetOperationCount() const
{
    return operations.size();
}

size_t MemoryOperator::GetAppliedCount() const
{
    size_t count = 0;
    for (const auto& [name, op] : operations) {
        if (op->is_modified) {
            count++;
        }
    }
    return count;
}

void MemoryOperator::PrintAllOperations() const
{
    std::cout << "\n=== Memory Operations (" << operations.size() << " total, "
        << GetAppliedCount() << " applied) ===\n";

    for (const auto& [name, op] : operations) {
        std::cout << "Name: " << name;

        if (dynamic_cast<Patch*>(op.get())) {
            std::cout << " | Type: Patch";
        }
        else if (dynamic_cast<WinDetour*>(op.get())) {
            std::cout << " | Type: Detour";
        }
        else {
            std::cout << " | Type: Unknown";
        }

        std::cout << " | Address: 0x" << std::hex << op->address;
        std::cout << " | Applied: " << (op->is_modified ? "Yes" : "No");
        std::cout << " | Size: " << std::dec << op->GetLength() << " bytes\n";
    }
    std::cout << "==========================================\n\n";
}

bool MemoryOperator::Exists(const std::string& name) const
{
    return operations.find(name) != operations.end();
}

