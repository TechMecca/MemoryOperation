#include "WinDetour.h"



WinDetour::WinDetour(PVOID* targetAddressRef, PVOID detourFunction)
{
    if (!targetAddressRef || !*targetAddressRef || !detourFunction) {
        throw std::invalid_argument("WinDetour: null targetAddressRef/*targetAddressRef or detourFunction");
    }

    // Detours wiring
    targetAddress = targetAddressRef;           // variable Detours will rewrite
    HookAddress = detourFunction;               // your hook
    targetStorage = *targetAddressRef;           // current value (real entry before attach)

    // Save bytes from the real entry (original function start)
    this->address = reinterpret_cast<uintptr_t>(targetStorage);

    size = 20;
    auto bytes = Memory::ReadBytes(this->address, size);


    original_bytes = std::move(bytes);

  
}



WinDetour::~WinDetour()
{
    // Best-effort restore; don't throw from a destructor.
    if (is_modified) {
        try { Restore(); }
        catch (...) {}
    }
}

bool WinDetour::Apply()
{
    if (is_modified) {
        std::cout << "Detour already applied\n";
        return true;
    }

    if (!targetStorage || !HookAddress) {
        std::cerr << "Apply: invalid state (targetStorage=" << targetStorage
            << ", HookAddress=0x" << std::hex << HookAddress << std::dec << ")\n";
        return false;
    }

    LONG rc = DetourTransactionBegin();
    if (rc != NO_ERROR) {
        std::cerr << "DetourTransactionBegin failed (rc=" << rc << ")\n";
        return false;
    }

    rc = DetourUpdateThread(GetCurrentThread());
    if (rc != NO_ERROR) {
        std::cerr << "DetourUpdateThread failed (rc=" << rc << ")\n";
        DetourTransactionAbort();
        return false;
    }

    rc = DetourAttach((PVOID*)targetAddress, (PVOID)HookAddress);
    if (rc != NO_ERROR) {
        std::cerr << "DetourAttach failed (rc=" << rc << ")\n";
        DetourTransactionAbort();
        return false;
    }

    rc = DetourTransactionCommit();
    if (rc != NO_ERROR) {
        std::cerr << "DetourTransactionCommit failed (rc=" << rc << ")\n";
        DetourTransactionAbort();
        return false;
    }

    // After commit, targetStorage now points to the trampoline (the original function).
    is_modified = true;

    std::cout << "Detour applied: target=0x" << std::hex << this->targetAddress
        << " trampoline=0x" << reinterpret_cast<uintptr_t>(targetStorage)
        << std::dec << "\n";
    return true;
}


bool WinDetour::Restore()
{
    if (!is_modified) {
        return true;
    }

    // Always mark as restored first to prevent multiple attempts
    is_modified = false;

    if (!IsValid()) {
        std::cout << "Restore: target memory invalid, skipping detach\n";
        return true;
    }

    __try {
        LONG rc = DetourTransactionBegin();
        if (rc == NO_ERROR) {
            DetourUpdateThread(GetCurrentThread());
            DetourDetach((PVOID*)targetAddress, (PVOID)HookAddress);
            DetourTransactionCommit();
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // Ignore all exceptions during restoration
    }

    std::cout << "Detour restoration attempted\n";
    return true;
}

//bool WinDetour::Restore()
//{
//    if (!is_modified) {
//        std::cout << "Detour not applied; nothing to restore\n";
//        return true;
//    }
//
//    if (!IsValid()) {
//        std::cout << "Restore: target memory is invalid - skipping detach\n";
//        is_modified = false;
//        return true;
//    }
//
//    LONG rc = DetourTransactionBegin();
//    if (rc != NO_ERROR) {
//        std::cerr << "DetourTransactionBegin failed (rc=" << rc << ")\n";
//        return false;
//    }
//
//    __try {
//        rc = DetourUpdateThread(GetCurrentThread());
//        if (rc != NO_ERROR) {
//            std::cerr << "DetourUpdateThread failed (rc=" << rc << ")\n";
//            DetourTransactionAbort();
//            return false;
//        }
//
//        rc = DetourDetach((PVOID*)targetAddress, (PVOID)HookAddress);
//        if (rc != NO_ERROR) {
//            // ERROR_NOT_ENOUGH_MEMORY (9) often means the detour was already removed
//            if (rc == ERROR_NOT_ENOUGH_MEMORY) {
//                std::cout << "DetourDetach: detour may already be removed (rc=9), continuing...\n";
//                // Don't return false here - try to commit anyway
//            }
//            else {
//                std::cerr << "DetourDetach failed (rc=" << rc << ")\n";
//                DetourTransactionAbort();
//                return false;
//            }
//        }
//
//        rc = DetourTransactionCommit();
//        if (rc != NO_ERROR) {
//            // Even if commit fails, the detour might already be gone
//            std::cout << "DetourTransactionCommit failed (rc=" << rc << "), but marking as restored\n";
//            DetourTransactionAbort();
//            // Don't return false - mark as restored anyway
//        }
//    }
//    __except (EXCEPTION_EXECUTE_HANDLER) {
//        const unsigned long code = GetExceptionCode();
//        std::cout << "Exception during Detour restore: code=0x" << std::hex << code << std::dec << "\n";
//    }
//
//    is_modified = false;
//    std::cout << "Detour restored: target=0x" << std::hex << this->targetAddress << std::dec << "\n";
//    return true;
//}


bool WinDetour::IsValid()
{
    if (targetAddress == nullptr || *targetAddress == nullptr || HookAddress == nullptr) {
        return false;
    }

    // Check if the memory is actually accessible and not filled with nulls
    __try {
        BYTE first_byte = *((BYTE*)(*targetAddress));
        // If first byte is 0x00, it's likely invalid memory
        if (first_byte == 0x00) {
            return false;
        }

        // Optional: Check a few more bytes to be more certain
        // for (int i = 0; i < 4; i++) {
        //     if (*((BYTE*)(*targetAddress) + i) == 0x00) {
        //         return false;
        //     }
        // }

        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // If we get an exception, the memory is not accessible
        return false;
    }
}
