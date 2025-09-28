#include "MemoryOperator.h"
#include <unordered_set>


bool MemoryOperator::DEBUG = false;

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

    try 
    {
        auto patch = std::make_unique<Patch>(address, bytes);
        Patch* ptr = patch.get();
        instance.operations[name] = std::move(patch);
        return ptr;
    }
    catch (const std::exception&) {
        return nullptr;
    }
}




WinDetour* MemoryOperator::CreateDetour(const std::string& name,
    uintptr_t target_addr,
    uintptr_t detour_addr,
    bool overrideExisting) 
{
    auto& inst = GetInstance();
    auto& ops = inst.operations;

    // name clash handling
    if (ops.contains(name)) {
        if (!overrideExisting) {
            return nullptr;
        }
        ops.erase(name);
    }

    try 
    {
        auto detour = std::make_shared<WinDetour>(
            reinterpret_cast<PVOID*>(target_addr),
            reinterpret_cast<PVOID>(detour_addr)
        );

        WinDetour* raw = detour.get();
        ops.emplace(name, std::move(detour));
        std::cout << "CreateDetour: created '" << name << "'\n";
        return raw;
    }
    catch (const std::exception& e) {
        std::cerr << "CreateDetour: exception for '" << name << "': " << e.what() << "\n";
        return nullptr;
    }
    catch (...) {
        std::cerr << "CreateDetour: unknown exception for '" << name << "'\n";
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


// i gotten the idea (code) From ByteWeaver / Oxkate
bool MemoryOperator::IsLocationModified(uintptr_t address, size_t length,
    std::map<std::string, std::shared_ptr<MemoryOperation>>& out)
{
    const auto end = address + length;
    auto&& ops = GetInstance().operations;
    std::ranges::copy_if(ops, std::inserter(out, out.end()),
        [=](const auto& kv) {
            const auto& op = kv.second;
            return op && op->is_modified &&
                address < op->address + op->size && end > op->address; // overlap
        });
    return !out.empty();
}



BOOL MemoryOperator::DisposeAll(bool saveActive, const std::vector<std::string>& ignoreList)
{
    auto& inst = GetInstance();
    auto& ops = inst.operations;
    auto* saved = saveActive ? &inst.Savedoperations : nullptr;

    if (saved) saved->clear();

    // Fast ignore lookups
    std::unordered_set<std::string_view> ignore;
    ignore.reserve(ignoreList.size());
    for (const auto& s : ignoreList) ignore.emplace(s);

    for (auto& [name, op] : ops) {
        if (!op || !op->is_modified) continue;
        if (ignore.find(name) != ignore.end()) continue;
        if (Memory::IsBadRange(op->address, op->size, /*write*/true)) continue;

        if (saved) saved->emplace(name, op);
        op->Restore();
    }

    return TRUE;
}


BOOL MemoryOperator::ApplyAll(bool useSavedActive) 
{
    auto& inst = GetInstance();
    auto& src = useSavedActive ? inst.Savedoperations : inst.operations;

    for (auto& [name, op] : src)
        if (op && (useSavedActive || !op->is_modified) &&
            !Memory::IsBadRange(op->address, op->size, true))
        {
            op->Apply();
        }

    return TRUE;
}




