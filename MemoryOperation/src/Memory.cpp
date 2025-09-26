#include "Memory.h"
#include <string>
#include <cstring>


uintptr_t Memory::GetBaseAddress()
{
    // Using W-variant; nullptr => current process main module
    HMODULE h = ::GetModuleHandleW(nullptr);
    return reinterpret_cast<uintptr_t>(h);
}

std::vector<unsigned char> Memory::ReadBytes(uintptr_t address, size_t size)
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


uintptr_t Memory::GetModuleAddress(std::string ModuleName)
{
    if (ModuleName.empty()) return 0;

    if (HMODULE h = ::GetModuleHandleA(ModuleName.c_str()))
        return reinterpret_cast<uintptr_t>(h);

    return NULL;
}

std::string Memory::BytesToString(const std::vector<uint8_t>& bytes, std::size_t len) {
    const std::size_t n = (std::min)(len, bytes.size());
    if (n == 0) return {};

    static const char* HEX = "0123456789ABCDEF";
    std::string out;
    out.reserve(n * 3 - 1); // "AA " per byte, minus last space

    for (std::size_t i = 0; i < n; ++i) {
        uint8_t b = bytes[i];
        out.push_back(HEX[b >> 4]);
        out.push_back(HEX[b & 0x0F]);
        if (i + 1 < n) out.push_back(' ');
    }
    return out;
}
