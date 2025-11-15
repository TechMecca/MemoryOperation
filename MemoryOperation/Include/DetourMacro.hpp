#include "MemoryOperator.h"


/**
 * Declare a hook with standard calling conventions.
 * @param Name The prefix for the macro, and string name of hook/patch.
 * @param Ret The return type. (ex. int)
 * @param CallType The calling convention of the original function. (ex. __cdecl)
 * @param HookCallType The calling convention of the hook. (ex. __fastcall)
 */
#define DECLARE_HOOK(Name, Ret, CallType, ...) \
    using Name##_t = Ret(CallType*)(__VA_ARGS__); \
    static inline uintptr_t Name##Address = 0; \
    static inline Name##_t Name##Original = nullptr; \
    static inline WinDetour* Name##Detour = nullptr; \
    static Ret CallType Name##Hook(__VA_ARGS__);


#define DECLARE_HOOK_THISCALL(Name, Ret, HookCallType, ...) \
    using Name##_t = Ret(__thiscall*)(const void* p_this, __VA_ARGS__); \
    static inline uintptr_t Name##Address{}; \
    static inline Name##_t Name##Original{}; \
    static inline WinDetour* Name##Detour{}; \
    static Ret HookCallType Name##Hook(const void* p_this, int edx, __VA_ARGS__);



 /**
  * Install a hook with a pre-defined address and apply it.
  * @param Name The prefix for the macro, and string name of hook/patch.
  * @param AddressValue The memory address of the function to hook.
  */
#define INSTALL_HOOK_ADDRESS(Name, AddressValue) \
    { \
        Name##Address = AddressValue; \
        Name##Original = (Name##_t)AddressValue; \
        Name##Detour = MemoryOperator::CreateDetour(#Name, (uintptr_t)&Name##Original, \
            (uintptr_t)Name##Hook, true); \
        \
        if (Name##Detour && !Name##Detour->Apply()) \
        { \
            std::cout << "[!] Failed to apply " << #Name << " detour\n"; \
        } \
        else if (Name##Detour) \
        { \
            std::cout << "[+] " << #Name << " detour applied at 0x" \
                      << std::hex << AddressValue << "\n"; \
        } \
    }