#include "MemoryOperator.h"
#include <unordered_set>

bool MemoryOperator::DEBUG = false;

std::map<std::string, std::shared_ptr<MemoryOperation>> MemoryOperator::operations;
std::map<std::string, std::shared_ptr<MemoryOperation>> MemoryOperator::Savedoperations;

// Helper: Check if detour memory is valid
bool MemoryOperator::IsDetourMemoryValid(WinDetour* detour)
{
    if (!detour || !detour->address) return false;
    return !IsBadReadPtr((void*)detour->address, sizeof(PVOID));
}

// Helper: Safely disable detour restoration
void MemoryOperator::SafeDisableDetourRestoration(WinDetour* detour)
{
    if (detour) {
        detour->is_modified = false;
        if (DEBUG) {
            std::cout << "Disabled restoration for detour (invalid memory)\n";
        }
    }
}

Patch* MemoryOperator::CreatePatch(const std::string& name,
    uintptr_t address,
    const std::vector<byte>& bytes)
{
    auto& ops = operations;

    // Check if name already exists
    if (ops.find(name) != ops.end()) {
        if (DEBUG) std::cerr << "CreatePatch: name '" << name << "' already exists\n";
        return nullptr;
    }

    // Basic validation
    if (!address || bytes.empty()) {
        if (DEBUG) std::cerr << "CreatePatch: invalid parameters\n";
        return nullptr;
    }

  

    auto patch = std::make_shared<Patch>(address, bytes);
    Patch* raw = patch.get();
    ops.emplace(name, std::static_pointer_cast<MemoryOperation>(std::move(patch)));

    return raw;
}

WinDetour* MemoryOperator::CreateDetour(const std::string& name,
    uintptr_t target_addr,
    uintptr_t detour_addr,
    bool overrideExisting)
{
    auto& ops = operations;

    // Handle name clash
    if (ops.contains(name)) {
        if (!overrideExisting) {
            if (DEBUG) std::cerr << "CreateDetour: name '" << name << "' already exists\n";
            return nullptr;
        }

        // Check if existing entry is a detour and if memory is still valid
        auto it = ops.find(name);
        if (it != ops.end()) {
            WinDetour* existingDetour = dynamic_cast<WinDetour*>(it->second.get());
            if (existingDetour && !IsDetourMemoryValid(existingDetour)) {
                SafeDisableDetourRestoration(existingDetour);
                if (DEBUG) std::cout << "Removing detour '" << name << "' without restoration (invalid memory)\n";
            }
        }

        ops.erase(name);
    }

    // Validate target memory before creating
    PVOID* target_ptr = reinterpret_cast<PVOID*>(target_addr);
    if (IsBadReadPtr(target_ptr, sizeof(PVOID))) {
        if (DEBUG) std::cerr << "CreateDetour: invalid target memory at 0x"
            << std::hex << target_addr << std::dec << "\n";
        return nullptr;
    }

    auto detour = std::make_shared<WinDetour>(target_ptr, reinterpret_cast<PVOID*>(detour_addr));
    WinDetour* raw = detour.get();
    ops.emplace(name, std::static_pointer_cast<MemoryOperation>(std::move(detour)));

    return raw;
}

Patch* MemoryOperator::FindPatch(const std::string& name)
{
    auto it = operations.find(name);
    if (it != operations.end()) {
        return dynamic_cast<Patch*>(it->second.get());
    }
    return nullptr;
}

WinDetour* MemoryOperator::FindDetour(const std::string& name)
{
    auto it = operations.find(name);
    if (it != operations.end()) {
        return dynamic_cast<WinDetour*>(it->second.get());
    }
    return nullptr;
}

bool MemoryOperator::IsLocationModified(uintptr_t address, size_t length,
    std::map<std::string, std::shared_ptr<MemoryOperation>>& out)
{
    const auto end = address + length;
    std::ranges::copy_if(operations, std::inserter(out, out.end()),
        [=](const auto& kv) {
            const auto& op = kv.second;
            return op && op->is_modified &&
                address < op->address + op->size && end > op->address;
        });
    return !out.empty();
}

BOOL MemoryOperator::DisposeAll(bool saveActive, const std::vector<std::string>& ignoreList)
{
    auto* saved = saveActive ? &Savedoperations : nullptr;
    if (saved) saved->clear();

    // Fast ignore lookups
    std::unordered_set<std::string_view> ignore;
    ignore.reserve(ignoreList.size());
    for (const auto& s : ignoreList) ignore.emplace(s);

    for (auto& [name, op] : operations) {
        if (!op || !op->is_modified) continue;
        if (ignore.find(name) != ignore.end()) continue;

        // Validate memory before restoration
        if (Memory::IsBadRange(op->address, op->size, true)) {
            // Memory is invalid - disable restoration for detours
            WinDetour* detour = dynamic_cast<WinDetour*>(op.get());
            if (detour) {
                SafeDisableDetourRestoration(detour);
            }
            continue;
        }

        if (saved) saved->emplace(name, op);
        op->Restore();
    }

    return TRUE;
}

BOOL MemoryOperator::ApplyAll(bool useSavedActive)
{
    auto& src = useSavedActive ? Savedoperations : operations;

    for (auto& [name, op] : src) {
        if (!op) continue;
        if (useSavedActive || !op->is_modified) {
            if (!Memory::IsBadRange(op->address, op->size, true)) {
                op->Apply();
            }
        }
    }

    return TRUE;
}

bool MemoryOperator::EraseAll()
{
    // First pass: safely restore what we can and disable restoration for invalid memory
    for (auto& [name, op] : operations) {
        if (!op) continue;

        if (op->is_modified) {
            if (!Memory::IsBadRange(op->address, op->size, true)) {
                op->Restore();
            }
            else {
                // Memory is invalid - disable restoration for detours
                WinDetour* detour = dynamic_cast<WinDetour*>(op.get());
                if (detour) {
                    SafeDisableDetourRestoration(detour);
                }
            }
            op->is_modified = false;
        }
    }

    // Now safe to clear everything
    operations.clear();
    Savedoperations.clear();

    return true;
}

bool MemoryOperator::Erase(const std::string& key)
{
    auto it = operations.find(key);
    if (it != operations.end()) {
        auto& op = it->second;

        // Check if it's a detour and if memory is still valid
        WinDetour* detour = dynamic_cast<WinDetour*>(op.get());
        if (detour && !IsDetourMemoryValid(detour)) {
            SafeDisableDetourRestoration(detour);
            if (DEBUG) std::cout << "Erasing detour '" << key << "' without restoration (invalid memory)\n";
        }

        operations.erase(it);
        return true;
    }
    return false;
}