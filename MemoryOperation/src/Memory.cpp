#include "Memory.h"
#include <string>
#include <cstring>


template<typename T>
T Memory::Read(uintptr_t address)
{
    static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");

    __try {
        return *reinterpret_cast<const T*>(address);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return T{};
    }
}

// 2. Read raw bytes (using memcpy) - SEH protected
std::vector<byte> Memory::ReadBytes(uintptr_t address, size_t size)
{
    std::vector<uint8_t> result(size);
    memcpy(result.data(), reinterpret_cast<void*>(address), size);
    return result;
}

// 3. Read ASCII string using ReadBytes
std::string Memory::ReadAscii(uintptr_t address, size_t max_length)
{
    if (address == 0)
        return "";

    auto bytes = ReadBytes(address, max_length);
    if (bytes.empty())
        return "";

    size_t length = 0;
    while (length < bytes.size() && bytes[length] != '\0') {
        length++;
    }

    return std::string(reinterpret_cast<const char*>(bytes.data()), length);
}

std::wstring Memory::ReadUnicode(uintptr_t address, size_t max_length)
{
    if (address == 0) return L"";

    size_t byte_size = max_length * sizeof(wchar_t);
    auto bytes = ReadBytes(address, byte_size);
    if (bytes.empty()) return L"";

    const wchar_t* wide_str = reinterpret_cast<const wchar_t*>(bytes.data());
    size_t length = 0;
    while (length < max_length && wide_str[length] != L'\0')
    {
        length++;
    }

    return std::wstring(wide_str, length);
}
