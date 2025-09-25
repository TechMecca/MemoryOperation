#include "Patch.h"
#include <stdexcept>
#include <cstring>

Patch::Patch(uintptr_t target_addr, const std::vector<byte>& bytes)
{
    if (target_addr == 0) {
        throw std::invalid_argument("Target address cannot be null");
    }

    if (bytes.empty()) {
        throw std::invalid_argument("Patch bytes cannot be empty");
    }

    address = target_addr;
    new_bytes = bytes;

    DWORD old_protection;
    if (!SetMemoryProtection(address, bytes.size(), PAGE_EXECUTE_READWRITE, &old_protection)) {
        throw std::runtime_error("Failed to change memory protection");
    }

    try 
    {
        // Backup original bytes
        original_bytes.resize(bytes.size());
        std::memcpy(original_bytes.data(), reinterpret_cast<const void*>(address), bytes.size());
    }
    catch (...) {
        // Restore protection on error
        DWORD temp;
        SetMemoryProtection(address, bytes.size(), old_protection, &temp);
        throw;
    }

    // Restore original protection
    DWORD temp;
    SetMemoryProtection(address, bytes.size(), old_protection, &temp);
}

Patch::~Patch()
{
    // Auto-restore on destruction if still modified
    if (is_modified) {
        Restore();
    }
}

bool Patch::Apply()
{
    if (is_modified || address == 0 || new_bytes.empty()) {
        return false;
    }

    DWORD old_protection;
    if (!SetMemoryProtection(address, new_bytes.size(), PAGE_EXECUTE_READWRITE, &old_protection)) {
        return false;
    }

    bool success = WriteMemory(address, new_bytes.data(), new_bytes.size());

    DWORD temp;
    SetMemoryProtection(address, new_bytes.size(), old_protection, &temp);

    if (success) {
        is_modified = true;
    }

    return success;
}

bool Patch::Restore()
{
    if (!is_modified || address == 0 || original_bytes.empty()) {
        return false;
    }

    DWORD old_protection;
    if (!SetMemoryProtection(address, original_bytes.size(), PAGE_EXECUTE_READWRITE, &old_protection)) {
        return false;
    }

    bool success = WriteMemory(address, original_bytes.data(), original_bytes.size());

    DWORD temp;
    SetMemoryProtection(address, original_bytes.size(), old_protection, &temp);

    if (success)
    {
        is_modified = false;
    }

    return success;
}


bool Patch::WriteMemory(uintptr_t addr, const void* data, size_t size)
{
    if (addr == 0 || data == nullptr || size == 0)
    {
        return false;
    }

    std::memcpy(reinterpret_cast<void*>(addr), data, size);
    return true;
}