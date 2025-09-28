#include "MemoryOperator.h"


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


WinDetour* MemoryOperator::CreateDetour(const std::string& name, uintptr_t target_addr, uintptr_t detour_addr, bool override = false)
{
    auto& instance = GetInstance();
    auto& ops = instance.operations;

    if (ops.find(name) != ops.end())
    {
        if (override)
        {
            std::cout << "CreateDetour(raw): overriding existing detour '" << name << "'\n";

            ops.erase(name);
        }
        else {
            std::cerr << "CreateDetour(raw): name '" << name << "' already exists\n";
            return nullptr;
        }
    }

    if (!target_addr || !detour_addr) {
        std::cerr << "CreateDetour(raw): null target/detour for '" << name << "'\n";
        return nullptr;
    }

    try {
        auto detour = std::make_shared<WinDetour>((PVOID*)target_addr, (PVOID)detour_addr);
        WinDetour* ptr = detour.get();
        ops.emplace(name, std::move(detour));
        std::cout << "CreateDetour(raw): successfully created detour '" << name << "'\n";
        return ptr;
    }
    catch (const std::exception& e) {
        std::cerr << "CreateDetour(raw): exception for '" << name << "': " << e.what() << "\n";
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


BOOL MemoryOperator::DisposeAll(bool saveActive) 
{
    auto& inst = GetInstance();
    auto& ops = inst.operations;
    auto* saved = saveActive ? &inst.Savedoperations : nullptr;

    if (saved) { saved->clear(); }

    for (auto& [name, op] : ops)
        if (op && op->is_modified && !Memory::IsBadRange(op->address, op->size, true))
        {
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
        if (op && (useSavedActive || op->is_modified) &&
            !Memory::IsBadRange(op->address, op->size, true))
        {
            op->Apply();
        }

    return TRUE;
}




