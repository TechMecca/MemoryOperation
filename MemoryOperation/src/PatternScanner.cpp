#include "PatternScanner.h"
#include <iostream>

Scanner::Scanner(uintptr_t Address, const std::string& pattern)
{
	this->startAddress = Address ? Address : reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
    if (!Scanner::ParsePattern(pattern))
    {
        std::cout << "Failed to parse pattern: " << pattern << std::endl;
    }      
}

Scanner::~Scanner()
{
    this->pattern.clear();
	this->mask.clear();
    this->startAddress = NULL;
}

// Add methods for pattern scanning here
bool Scanner::ParsePattern(const std::string& pattern)
{
    std::istringstream stream(pattern);
    std::string byteStr;

    while (stream >> byteStr)
    {
        if (byteStr == "?" || byteStr == "??")
        {
            this->pattern.push_back(0);
            mask.push_back(false); // wildcard
        }
        else
        {
            try
            {
                this->pattern.push_back(static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16)));
                mask.push_back(true); // must match
            }
            catch (...) {

                return false;
            }
        }
    }
    return !this->pattern.empty();
}

bool Scanner::Scan(uintptr_t* results)
{
    if (this->pattern.empty() || results == nullptr) return false;

    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    const uintptr_t endAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

    MEMORY_BASIC_INFORMATION mbi{};
    uintptr_t currentAddress = startAddress;

    while (currentAddress < endAddress &&
        VirtualQuery(reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)))
    {
        // Only scan committed, readable memory
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) &&
            !(mbi.Protect & PAGE_GUARD))
        {
            uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            uintptr_t regionEnd = regionStart + mbi.RegionSize;

            // Scan this memory region
            for (uintptr_t addr = regionStart; addr < regionEnd - pattern.size(); ++addr)
            {
                bool found = true;
                const uint8_t* data = reinterpret_cast<const uint8_t*>(addr);

                for (size_t i = 0; i < pattern.size(); ++i)
                {
                    // Skip wildcard bytes
                    if (!mask[i]) continue;

                    // Compare byte
                    if (data[i] != pattern[i]) {
                        found = false;
                        break;
                    }
                }

                if (found)
                {
                    *results = addr;
                    return true;
                }
            }
        }
        currentAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (currentAddress < reinterpret_cast<uintptr_t>(mbi.BaseAddress)) {
            break;
        }
    }

    return false; 
}

bool Scanner::Scan(uintptr_t* results, bool scanForFunction)
{
    if (this->pattern.empty() || results == nullptr) return false;

    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    const uintptr_t startAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    const uintptr_t endAddress = reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress);

    MEMORY_BASIC_INFORMATION mbi{};
    uintptr_t currentAddress = startAddress;

    while (currentAddress < endAddress &&
        VirtualQuery(reinterpret_cast<LPCVOID>(currentAddress), &mbi, sizeof(mbi)))
    {
        // Determine memory protection based on what we're scanning for
        bool shouldScan = false;

        if (mbi.State == MEM_COMMIT && !(mbi.Protect & PAGE_GUARD))
        {
            if (scanForFunction)
            {
                // For functions: look in executable memory
                shouldScan = (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE));
            }
            else
            {
                // For variables: look in readable/writable memory (non-executable)
                shouldScan = (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_READWRITE | PAGE_WRITECOPY)) &&
                    !(mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE));
            }
        }

        if (shouldScan)
        {
            uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            uintptr_t regionEnd = regionStart + mbi.RegionSize;
            size_t regionSize = mbi.RegionSize;

            // Safety check to avoid overflow
            if (regionSize > pattern.size() && regionEnd > regionStart)
            {
                // Scan this memory region
                for (uintptr_t addr = regionStart; addr < regionEnd - pattern.size(); ++addr)
                {
                    bool found = true;
                    const uint8_t* data = reinterpret_cast<const uint8_t*>(addr);

                    for (size_t i = 0; i < pattern.size(); ++i)
                    {
                        // Skip wildcard bytes (assuming mask is available)
                        if (!mask[i]) continue;

                        // Compare byte
                        if (data[i] != pattern[i]) {
                            found = false;
                            break;
                        }
                    }

                    if (found)
                    {
                        *results = addr;
                        return true;
                    }
                }
            }
        }

        currentAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;

        // Check for overflow/wrap-around
        if (currentAddress < reinterpret_cast<uintptr_t>(mbi.BaseAddress)) {
            break;
        }
    }

    return false;
}

