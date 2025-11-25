#include "MemoryOperation.h"

bool MemoryOperation::SetMemoryProtection(uintptr_t addr, size_t size, DWORD new_protection, DWORD* old_protection)
{
	return VirtualProtect(reinterpret_cast<LPVOID>(addr), size, new_protection, old_protection) != 0;
}