#pragma once
#include <windows.h>
#include <cstdint>

/// Scans a specific module's memory for a pattern (moduleBase + offset)
uintptr_t FindPattern(HANDLE hProc, DWORD procId, uintptr_t moduleBase, const char* pattern, const char* mask);

/// Scans the entire process memory for a pattern (no module base required)
uintptr_t FindPatternEx(HANDLE hProc, const char* pattern, const char* mask);



