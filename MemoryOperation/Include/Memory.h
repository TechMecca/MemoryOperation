#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>   // <-- for MODULEENTRY32, CreateToolhelp32Snapshot, Module32First/Next
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <cstdint>
#include <vector>

class Memory
{
public:

    static uintptr_t GetBaseAddress();

    static uintptr_t GetModuleAddress(std::string ModuleName);


    template<typename T>
    static T Read(uintptr_t address);

    // 2. Read raw bytes
    static std::vector<unsigned char> ReadBytes(uintptr_t address, size_t size);

    // 3. Read ASCII string (null-terminated)
    static std::string ReadAscii(uintptr_t address, size_t max_length = 256);

    // 4. Read Unicode string (null-terminated)
    static std::wstring ReadUnicode(uintptr_t address, size_t max_length = 256);
    static std::string BytesToString(const std::vector<uint8_t>& bytes, std::size_t len);

    static BOOL IsBadRange(uintptr_t addr, size_t len, bool write);
  
};

template<typename T>
static T Memory::Read(std::uintptr_t address) {

    if(IsBadReadPtr(reinterpret_cast<void*>(address), sizeof(T)) || address == NULL)
		return T{};
    __try 
    {
        return *reinterpret_cast<T*>(address);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return T{};
    }
}