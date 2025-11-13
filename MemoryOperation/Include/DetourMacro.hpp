#include "MemoryOperator.h"

/**
 * Declare a hook with standard calling conventions.
 * @param Name The prefix for the macro, and string name of hook/patch.
 * @param Ret The return type. (ex. int)
 * @param CallType The calling convention of the original function. (ex. __cdecl)
 * @param HookCallType The calling convention of the hook. (ex. __fastcall)
 */
#define DECLARE_HOOK(Name, Ret, CallType, HookCallType, ...) \
    using Name##_t = Ret(CallType*)(__VA_ARGS__); \
    static inline uintptr_t Name##Address{}; \
    static inline Name##_t Name##Original{}; \
    static Ret HookCallType Name##Hook(__VA_ARGS__);

 /**
  * Declare a hook for __thiscall methods. Automatically adds THIS and EDX params.
  * @param Name The prefix for the macro, and string name of hook/patch.
  * @param Ret The return type. (ex. int)
  * @param HookCallType The calling convention of the hook. (ex. __fastcall)
  */
#define DECLARE_HOOK_THISCALL(Name, Ret, HookCallType, ...) \
    using Name##_t = Ret(__thiscall*)(const void* p_this, __VA_ARGS__); \
    static inline uintptr_t Name##Address{}; \
    static inline Name##_t Name##Original{}; \
    static Ret HookCallType Name##Hook(const void* p_this, int edx, __VA_ARGS__);

  /**
   * Install a hook using symbols from AddressDB.
   * @param Name The prefix for the macro, and string name of hook/patch.
   * @param Symbol The exact function name as entered in AddressDB. (ex. lua_gettop)
   * @param Module The exact module name in AddressDB. (ex. lua514.dll)
   */
#define INSTALL_HOOK_SYMBOL(Name, Symbol, Module) \
    { \
        if (const auto _sym = AddressDB::Find(Symbol, Module)) \
        { \
            Name##Address = _sym->GetAddress().value(); \
            Name##Original = reinterpret_cast<Name##_t>(Name##Address); \
            MemoryOperator::CreateDetour(#Name, Name##Address, \
                &reinterpret_cast<PVOID&>(Name##Original), \
                reinterpret_cast<void*>(Name##Hook)); \
            \
            std::cout << "[" << #Name << "] Resolved " << #Symbol \
                      << " at 0x" << std::hex << Name##Address << "\n"; \
        } \
        else \
        { \
            std::cout << "[!] [" << #Name << "] Could not find " << #Symbol \
                      << " in " << #Module << "\n"; \
        } \
    }

   /**
    * Install a hook with a pre-defined address.
    * @param Name The prefix for the macro, and string name of hook/patch.
    * @param AddressValue The memory address of the function to hook.
    */
#define INSTALL_HOOK_ADDRESS(Name, AddressValue) \
    { \
        Name##Address = AddressValue; \
        Name##Original = reinterpret_cast<Name##_t>(Name##Address); \
        MemoryOperator::CreateDetour(#Name, Name##Address, \
            &reinterpret_cast<PVOID&>(Name##Original), \
            reinterpret_cast<void*>(Name##Hook)); \
        \
        std::cout << "[" << #Name << "] Installed at 0x" \
                  << std::hex << AddressValue << "\n"; \
    }

    /**
     * Install a hook with a function pointer directly.
     * @param Name The prefix for the macro, and string name of hook/patch.
     * @param FuncPtr The function pointer to the original function.
     */
#define INSTALL_HOOK_FUNCPTR(Name, FuncPtr) \
    { \
        Name##Address = reinterpret_cast<uintptr_t>(FuncPtr); \
        Name##Original = reinterpret_cast<Name##_t>(FuncPtr); \
        MemoryOperator::CreateDetour(#Name, Name##Address, \
            &reinterpret_cast<PVOID&>(Name##Original), \
            reinterpret_cast<void*>(Name##Hook)); \
        \
        std::cout << "[" << #Name << "] Installed at 0x" \
                  << std::hex << Name##Address << "\n"; \
    }

     /**
      * Install a hook with a pre-defined address and apply it.
      * @param Name The prefix for the macro, and string name of hook/patch.
      * @param AddressValue The memory address of the function to hook.
      */
#define INSTALL_HOOK_ADDRESS(Name, AddressValue) \
    { \
        Name##Address = AddressValue; \
        Name##Original = reinterpret_cast<Name##_t>(Name##Address); \
        WinDetour* Name##Detour = MemoryOperator::CreateDetour(#Name, Name##Address, \
            &reinterpret_cast<PVOID&>(Name##Original), \
            reinterpret_cast<void*>(Name##Hook)); \
        \
        if (Name##Detour && Name##Detour->Apply()) \
        { \
            std::cout << "[+] [" << #Name << "] Installed at 0x" \
                      << std::hex << AddressValue << "\n"; \
        } \
        else \
        { \
            std::cout << "[!] [" << #Name << "] Failed to apply detour at 0x" \
                      << std::hex << AddressValue << "\n"; \
        } \
    }