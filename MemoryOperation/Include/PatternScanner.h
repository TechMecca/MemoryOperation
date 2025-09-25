#pragma once
#include <Windows.h>
#include <sstream>
#include <vector>


class Scanner
{
public:
	Scanner(uintptr_t Address, const std::string& pattern);
	~Scanner();
	bool Scan(uintptr_t* results);

private:
	std::vector<uint8_t> pattern;
	std::vector<bool> mask;
	uintptr_t startAddress;
	bool ParsePattern(const std::string& pattern);

};
