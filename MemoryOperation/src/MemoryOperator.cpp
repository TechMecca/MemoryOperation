#include "MemoryOperator.h"
#include <iostream>
#include <typeinfo>

MemoryOperator& MemoryOperator::GetInstance()
{
    static MemoryOperator instance;
    return instance;
}

bool MemoryOperator::AddPatch(const std::string& name, uintptr_t address, const std::vector<byte>& bytes)
{
    if (Exists(name)) 
    {
        std::cout << "Operation '" << name << "' already exists\n";
        return false;
    }

    if (address == 0 || bytes.empty()) {
        std::cout << "Invalid parameters for patch '" << name << "'\n";
        return false;
    }

    try {
        auto patch = std::make_shared<Patch>(address, bytes);
        operations[name] = patch;
        std::cout << "Patch '" << name << "' added successfully at 0x" << std::hex << address << std::dec << "\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create patch '" << name << "': " << e.what() << std::endl;
        return false;
    }
}

bool MemoryOperator::AddDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr)
{
    if (Exists(name)) {
        std::cout << "Operation '" << name << "' already exists\n";
        return false;
    }

    if (target_addr == 0 || detour_addr == 0) {
        std::cout << "Invalid parameters for detour '" << name << "'\n";
        return false;
    }

    try {
        auto detour = std::make_shared<WinDetour>(target_addr, detour_addr);
        operations[name] = detour;
        std::cout << "Detour '" << name << "' added successfully (target: 0x" << std::hex << target_addr
            << ", detour: 0x" << detour_addr << ")\n" << std::dec;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create detour '" << name << "': " << e.what() << std::endl;
        return false;
    }
}

bool MemoryOperator::RemoveOperation(const std::string& name)
{
    auto it = operations.find(name);
    if (it != operations.end()) {
        // Auto-restore if applied
        if (it->second->is_modified) {
            it->second->Restore();
        }
        operations.erase(it);
        std::cout << "Operation '" << name << "' removed successfully\n";
        return true;
    }
    std::cout << "Operation '" << name << "' not found\n";
    return false;
}

void MemoryOperator::RemoveAllOperations()
{
    for (auto& [name, op] : operations) {
        if (op->is_modified) {
            op->Restore();
        }
    }
    operations.clear();
    std::cout << "All operations removed\n";
}

std::shared_ptr<MemoryOperation> MemoryOperator::FindOperation(const std::string& name)
{
    auto it = operations.find(name);
    if (it != operations.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Patch> MemoryOperator::FindPatch(const std::string& name)
{
    auto op = FindOperation(name);
    if (op) {
        return std::dynamic_pointer_cast<Patch>(op);
    }
    return nullptr;
}

std::shared_ptr<WinDetour> MemoryOperator::FindDetour(const std::string& name)
{
    auto op = FindOperation(name);
    if (op) {
        return std::dynamic_pointer_cast<WinDetour>(op);
    }
    return nullptr;
}

bool MemoryOperator::ApplyOperation(const std::string& name)
{
    auto op = FindOperation(name);
    if (!op) {
        std::cout << "Operation '" << name << "' not found\n";
        return false;
    }

    if (op->Apply()) {
        std::cout << "Operation '" << name << "' applied successfully\n";
        return true;
    }

    std::cout << "Failed to apply operation '" << name << "'\n";
    return false;
}

bool MemoryOperator::RestoreOperation(const std::string& name)
{
    auto op = FindOperation(name);
    if (!op)
    {
        std::cout << "Operation '" << name << "' not found\n";
        return false;
    }

    if (op->Restore()) {
        std::cout << "Operation '" << name << "' restored successfully\n";
        return true;
    }

    std::cout << "Failed to restore operation '" << name << "'\n";
    return false;
}

bool MemoryOperator::ApplyAll()
{
    bool all_success = true;
    for (auto& [name, op] : operations) {
        if (!op->is_modified) {
            if (!op->Apply()) {
                std::cout << "Failed to apply operation '" << name << "'\n";
                all_success = false;
            }
        }
    }
    return all_success;
}

bool MemoryOperator::RestoreAll()
{
    bool all_success = true;
    for (auto& [name, op] : operations) {
        if (op->is_modified) {
            if (!op->Restore()) {
                std::cout << "Failed to restore operation '" << name << "'\n";
                all_success = false;
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
    std::cout << "\n=== Memory Operations (" << operations.size() << " total) ===\n";
    for (const auto& [name, op] : operations) {
        std::cout << "Name: " << name;
        std::cout << " | Type: " << (dynamic_cast<Patch*>(op.get()) ? "Patch" : "Detour");
        std::cout << " | Address: 0x" << std::hex << op->address;
        std::cout << " | Applied: " << (op->is_modified ? "Yes" : "No");
        std::cout << " | Size: " << std::dec << op->GetLength() << " bytes\n";
    }
    std::cout << "===================================\n\n";
}

bool MemoryOperator::Exists(const std::string& name) const
{
    return operations.find(name) != operations.end();
}