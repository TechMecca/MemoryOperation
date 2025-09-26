#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <type_traits>

class Memory
{
public:
    // 1. Read any type with template (dereferences pointer)
    template<typename T>
    static T Read(uintptr_t address);

    // 2. Read raw bytes
    static std::vector<byte> ReadBytes(uintptr_t address, size_t size);

    // 3. Read ASCII string (null-terminated)
    static std::string ReadAscii(uintptr_t address, size_t max_length = 256);

    // 4. Read Unicode string (null-terminated)
    static std::wstring ReadUnicode(uintptr_t address, size_t max_length = 256);
};

template<typename T>
static T Memory::Read(std::uintptr_t address) {
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
    __try {
        return *reinterpret_cast<const T*>(address);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return T{};
    }
}