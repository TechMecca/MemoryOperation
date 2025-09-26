#include "Patch.h"
#include <stdexcept>
#include <cstring>
#include <iostream>

Patch::Patch(uintptr_t target_addr, const std::vector<byte>& bytes)
{
    if (target_addr == 0)
    {
        throw std::invalid_argument("Target address cannot be null");
    }

    if (bytes.empty()) {
        throw std::invalid_argument("Patch bytes cannot be empty");
    }

    address = target_addr;
    new_bytes = bytes;
    size = 23;

    // Change protection to read original bytes
    DWORD old_protection;
    if (!SetMemoryProtection(address, size, PAGE_EXECUTE_READWRITE, &old_protection)) {
        throw std::runtime_error("Failed to change memory protection for backup");
    }

    // Backup original bytes
    original_bytes.resize(23);
    std::memcpy(original_bytes.data(), reinterpret_cast<const void*>(address), 23);

    // Restore original protection
    DWORD temp;
    if (!SetMemoryProtection(address, size, old_protection, &temp)) {
        std::cerr << "Warning: Failed to restore original memory protection" << std::endl;
    }

    std::cout << "Patch prepared at 0x" << std::hex << address << std::dec
        << " (size: " << size << " bytes)" << std::endl;
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
        std::cout << "\x1b[91m[Patch] Apply failed: Already applied or invalid state\x1b[0m" << std::endl;
        return false;
    }

    DWORD old_protection;
    if (!SetMemoryProtection(address, new_bytes.size(), PAGE_EXECUTE_READWRITE, &old_protection)) {
        std::cout << "\x1b[91m[Patch] Apply failed: Memory protection change failed\x1b[0m" << std::endl;
        return false;
    }

    bool success = WriteMemory(address, new_bytes.data(), new_bytes.size());

    DWORD temp;
    SetMemoryProtection(address, new_bytes.size(), old_protection, &temp);

    if (success) {
        is_modified = true;
        std::cout << "\x1b[92m[Patch] Applied successfully at 0x" << std::hex << address
            << " (" << std::dec << new_bytes.size() << " bytes)\x1b[0m" << std::endl;
    }
    else {
        std::cout << "\x1b[91m[Patch] Apply failed: Memory write failed\x1b[0m" << std::endl;
    }

    return success;
}

bool Patch::Restore()
{
    if (!is_modified || address == 0 || original_bytes.empty()) {
        std::cout << "\x1b[93m[Patch] Restore failed: Not applied or invalid state\x1b[0m" << std::endl;
        return false;
    }

    DWORD old_protection;
    if (!SetMemoryProtection(address, original_bytes.size(), PAGE_EXECUTE_READWRITE, &old_protection)) {
        std::cout << "\x1b[91m[Patch] Restore failed: Memory protection change failed\x1b[0m" << std::endl;
        return false;
    }

    bool success = WriteMemory(address, original_bytes.data(), new_bytes.size());

    DWORD temp;
    SetMemoryProtection(address, original_bytes.size(), old_protection, &temp);

    if (success) {
        is_modified = false;
        std::cout << "\x1b[92m[Patch] Restored successfully at 0x" << std::hex << address
            << " (" << std::dec << original_bytes.size() << " bytes)\x1b[0m" << std::endl;
    }
    else {
        std::cout << "\x1b[91m[Patch] Restore failed: Memory write failed\x1b[0m" << std::endl;
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