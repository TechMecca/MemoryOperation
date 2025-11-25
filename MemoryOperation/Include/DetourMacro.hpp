#include "MemoryOperator.h"

/**
 * Declare a hook with standard calling conventions.
 */
#define DECLARE_HOOK(Name, Ret, CallType, ...) \
    using Name##_t = Ret(CallType*)(__VA_ARGS__); \
    static inline uintptr_t Name##Address = 0; \
    static inline Name##_t Name##Original = nullptr; \
    static inline std::string Name##DetourKey = #Name; \
    static Ret CallType Name##Hook(__VA_ARGS__);

#define DECLARE_HOOK_THISCALL(Name, Ret, HookCallType, ...) \
    using Name##_t = Ret(__thiscall*)(const void* p_this, __VA_ARGS__); \
    static inline uintptr_t Name##Address{}; \
    static inline Name##_t Name##Original{}; \
    static inline std::string Name##DetourKey = #Name; \
    static Ret HookCallType Name##Hook(const void* p_this, int edx, __VA_ARGS__);

#define DECLARE_HOOK_DYNAMIC_THISCALL(Name, Ret, HookCallType, StructType, ...) \
    using Name##_t = Ret(__thiscall*)(StructType p_this, __VA_ARGS__); \
    static inline uintptr_t Name##Address{}; \
    static inline Name##_t Name##Original{}; \
    static inline std::string Name##DetourKey = #Name; \
    static Ret HookCallType Name##Hook(StructType p_this, int edx, __VA_ARGS__);

 /**
  * Install a hook with a pre-defined address and apply it.
  * Returns true on success, false on failure.
  */
#define INSTALL_HOOK_ADDRESS(Name, AddressValue) \
    [&]() -> bool { \
        if (!AddressValue) { \
            std::cout << "[!] Invalid address for " << #Name << "\n"; \
            return false; \
        } \
        \
        if (IsBadReadPtr((void*)AddressValue, 1)) { \
            std::cout << "[!] Unreadable memory at address 0x" \
                      << std::hex << AddressValue << " for " << #Name << "\n"; \
            return false; \
        } \
        \
        Name##Address = AddressValue; \
        Name##Original = (Name##_t)AddressValue; \
        \
        WinDetour* detour = MemoryOperator::CreateDetour( \
            Name##DetourKey, \
            (uintptr_t)&Name##Original, \
            (uintptr_t)Name##Hook, \
            true \
        ); \
        \
        if (!detour) { \
            std::cout << "[!] Failed to create " << #Name << " detour\n"; \
            return false; \
        } \
        \
        if (!detour->Apply()) { \
            std::cout << "[!] Failed to apply " << #Name << " detour\n"; \
            MemoryOperator::Erase(Name##DetourKey); \
            return false; \
        } \
        \
        std::cout << "[+] " << #Name << " detour applied at 0x" \
                  << std::hex << AddressValue << std::dec << "\n"; \
        return true; \
    }()

  /**
   * Safely get the detour pointer (checks if it still exists)
   */
#define GET_HOOK_DETOUR(Name) \
    MemoryOperator::FindDetour(Name##DetourKey)

   /**
    * Remove a hook safely
    */
#define REMOVE_HOOK(Name) \
    [&]() -> bool { \
        WinDetour* detour = MemoryOperator::FindDetour(Name##DetourKey); \
        if (detour) { \
            return MemoryOperator::Erase(Name##DetourKey); \
        } \
        return false; \
    }()

    /**
     * Check if a hook is currently active
     */
#define IS_HOOK_ACTIVE(Name) \
    [&]() -> bool { \
        WinDetour* detour = MemoryOperator::FindDetour(Name##DetourKey); \
        return detour && detour->is_modified; \
    }()