#pragma once
#include "MemoryOperation.h"

class Patch : public MemoryOperation
{
public:
    Patch(uintptr_t target_addr, const std::vector<byte>& bytes);
    ~Patch();

    bool Apply() override;
    bool Restore() override;
    size_t GetLength() const override { return new_bytes.size(); }

private:
    bool WriteMemory(uintptr_t address, const void* data, size_t size);
};