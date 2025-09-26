#pragma once
#include <Windows.h>
#include <vector>

typedef unsigned char byte;

class MemoryOperation
{
public:
    uintptr_t address = 0;
    std::vector<uint8_t> original_bytes{};
    std::vector<uint8_t> new_bytes{};
    SIZE_T size = 0;
    bool is_modified = false;

    virtual ~MemoryOperation() = default;
    virtual bool Apply() = 0;
    virtual bool Restore() = 0;
    virtual size_t GetLength() const { return new_bytes.size(); }
    bool SetMemoryProtection(uintptr_t addr, size_t size, DWORD new_protection, DWORD* old_protection);
};