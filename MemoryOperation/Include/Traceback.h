#pragma once
#include "Windows.h"
#include <cstdint>
#include <stdio.h>
#include <array>
#include <vector>


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
            std::cout << "----------------------------------------" << std::endl;
            for (FrameInfo frame : Frames) {
                frame.Dump();
            }
            std::cout << "----------------------------------------" << std::endl;
        }
    };

    static TraceInfo Capture(const USHORT skip = 1, USHORT maxFrames = 62) {
        if (maxFrames > 62) maxFrames = 62;

        auto traceback = TraceInfo{};

        traceback.StackSize = RtlCaptureStackBackTrace(skip, maxFrames, traceback.Stack.data(), nullptr);
        traceback.Frames.reserve(traceback.StackSize);

        for (USHORT i = 0; i < traceback.StackSize; ++i) {
            traceback.Frames.push_back(FrameInfo{
                reinterpret_cast<uintptr_t>(traceback.Stack[i]),
                i
                });
        }
        return traceback;
    }
};