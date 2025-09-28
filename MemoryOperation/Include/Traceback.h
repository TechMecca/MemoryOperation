#pragma once
#include "Windows.h"
#include <cstdint>
#include <stdio.h>
#include <array>
#include <vector>


#pragma once
#include "Windows.h"
#include <cstdint>
#include <stdio.h>
#include <array>
#include <vector>
#include <iostream>


class Traceback {
public:
    struct FrameInfo {
        uintptr_t CallAddress{};
        USHORT    StackIndex{};
        void Dump() const
        {
            char msg[1024];
            _snprintf_s(msg, sizeof(msg), _TRUNCATE,
                "[FrameInfo] %-2u) - " "0x%08x",
                StackIndex, CallAddress);

            std::cout << msg << std::endl;
        }

    };

    struct TraceInfo {
        USHORT StackSize{};
        std::array<void*, 62> Stack{};
        std::vector<FrameInfo> Frames{};
        void Dump() const {
            for (FrameInfo frame : Frames) {
                frame.Dump();
            }
        }
    };


    static USHORT ScanStackMemory(USHORT skip, USHORT maxFrames, void** addresses) {
        USHORT count = 0;
        DWORD* stackPtr;

        __asm mov stackPtr, esp;

        std::cout << "Starting stack scan from ESP: 0x" << stackPtr << std::endl;

        DWORD* scanStart = stackPtr;
        DWORD* scanEnd = stackPtr + 1024; // Scan up to 4KB of stack

        std::cout << "Scanning from 0x" << scanStart << " to 0x" << scanEnd << std::endl;

        for (DWORD* ptr = scanStart; ptr < scanEnd && count < maxFrames; ptr++) {
            if (IsBadReadPtr(ptr, 4)) {
                std::cout << "Bad read at 0x" << ptr << ", stopping scan" << std::endl;
                break;
            }

            if (DWORD addr = *ptr; addr >= 0x00400000 && addr <= 0x7FFFFFFF) {
                if (!IsBadReadPtr(reinterpret_cast<void*>(addr), 1)) {
                    if (addr > 1 && !IsBadReadPtr(reinterpret_cast<void*>(addr - 1), 1)) {
                        if (auto codePtr = reinterpret_cast<BYTE*>(addr - 1); *codePtr == 0xE8 || *codePtr == 0xFF || *codePtr == 0x9A) {
                            std::cout << "Found potential return address at stack offset " << ((ptr - stackPtr) * 4)
                                << ": 0x" << reinterpret_cast<void*>(addr) << std::endl;

                            if (skip > 0) {
                                skip--;
                                std::cout << "Skipping this address (remaining skip: " << skip << ")" << std::endl;
                            }
                            else {
                                addresses[count++] = reinterpret_cast<void*>(addr);
                                std::cout << "Added address " << (count - 1) << ": 0x" << reinterpret_cast<void*>(addr) << std::endl;
                            }
                        }
                    }
                }
            }
        }

        std::cout << "Stack memory scan found " << count << " addresses" << std::endl;
        return count;
    }

    static TraceInfo Capture(const USHORT skip = 1, USHORT maxFrames = 62) {
        if (maxFrames > 62) maxFrames = 62;

        auto traceback = TraceInfo{};

        traceback.StackSize = ScanStackMemory(skip, maxFrames, traceback.Stack.data());

        traceback.Frames.reserve(traceback.StackSize);
        std::cout << "Final stacktrace size: " << traceback.StackSize << std::endl;

        for (USHORT i = 0; i < traceback.StackSize; ++i) {
            std::cout << "Frame " << i << ": 0x" << traceback.Stack[i] << std::endl;
            traceback.Frames.push_back(FrameInfo{
                reinterpret_cast<uintptr_t>(traceback.Stack[i]),
                i
                });
        }

        return traceback;
    }
};