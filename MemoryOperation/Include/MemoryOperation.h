#pragma once
#include <Windows.h>
#include <vector>

typedef unsigned char byte;

class MemoryOperation
{
public:
    uintptr_t address = 0;
    std::vector<byte> original_bytes{};
    std::vector<byte> new_bytes{};
    bool is_modified = false;

    virtual ~MemoryOperation() = default;
    virtual bool Apply() = 0;
    virtual bool Restore() = 0;
    virtual size_t GetLength() const { return new_bytes.size(); }
    bool SetMemoryProtection(uintptr_t addr, size_t size, DWORD new_protection, DWORD* old_protection);
};