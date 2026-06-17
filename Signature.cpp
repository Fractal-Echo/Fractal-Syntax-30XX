#include "Signature.h"
#include <vector>
#include <iostream>
#include <string>
#include <tlhelp32.h>

/// Internal: Match pattern with nibble + wildcard support
static bool MatchPattern(const unsigned char* pattern, const char* mask, const unsigned char* data)
{
    for (; *mask; ++mask, ++pattern, ++data)
    {
        if (*mask == '?')
            continue;

        if (*mask == 'n') // nibble match
        {
            unsigned char ph = (*pattern & 0xF0);
            unsigned char pl = (*pattern & 0x0F);

            if ((ph && ph != (*data & 0xF0)) ||
                (pl && pl != (*data & 0x0F)))
            {
                return false;
            }

            continue;
        }

        if (*pattern != *data)
            return false;
    }

    return true;
}

/// Helper: Get module size
static size_t GetModuleSize(DWORD procId, uintptr_t moduleBase)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32W modEntry{};
    modEntry.dwSize = sizeof(modEntry);

    if (!Module32FirstW(hSnap, &modEntry))
    {
        CloseHandle(hSnap);
        return 0;
    }

    do
    {
        if ((uintptr_t)modEntry.modBaseAddr == moduleBase)
        {
            size_t size = (size_t)modEntry.modBaseSize;
            CloseHandle(hSnap);
            return size;
        }

    } while (Module32NextW(hSnap, &modEntry));

    CloseHandle(hSnap);
    return 0;
}

/// Scan inside a specific module
uintptr_t FindPattern(HANDLE hProc, DWORD procId, uintptr_t moduleBase, const char* pattern, const char* mask)
{
    if (!hProc || !pattern || !mask)
        return 0;

    size_t moduleSize = GetModuleSize(procId, moduleBase);
    if (!moduleSize)
        return 0;

    std::vector<unsigned char> buffer(moduleSize);
    SIZE_T bytesRead;

    if (!ReadProcessMemory(hProc, (LPCVOID)moduleBase, buffer.data(), moduleSize, &bytesRead))
        return 0;

    size_t patternLen = strlen(mask);

    for (size_t i = 0; i < bytesRead - patternLen; i++)
    {
        if (MatchPattern((unsigned char*)pattern, mask, buffer.data() + i))
            return moduleBase + i;
    }

    return 0;
}

/// Scan entire process memory
uintptr_t FindPatternEx(HANDLE hProc, const char* pattern, const char* mask)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    uintptr_t start = (uintptr_t)sysInfo.lpMinimumApplicationAddress;
    uintptr_t end = (uintptr_t)sysInfo.lpMaximumApplicationAddress;

    MEMORY_BASIC_INFORMATION mbi{};
    size_t patternLen = strlen(mask);

    for (uintptr_t addr = start; addr < end; addr += mbi.RegionSize)
    {
        if (!VirtualQueryEx(hProc, (LPCVOID)addr, &mbi, sizeof(mbi)))
            continue;

        if (mbi.State != MEM_COMMIT)
            continue;

        if (mbi.Protect & PAGE_GUARD)
            continue;

        if (!(mbi.Protect & (PAGE_READWRITE |
            PAGE_READONLY |
            PAGE_EXECUTE_READ |
            PAGE_EXECUTE_READWRITE)))
            continue;

        std::vector<unsigned char> buffer(mbi.RegionSize);
        SIZE_T bytesRead;

        if (!ReadProcessMemory(hProc, (LPCVOID)addr, buffer.data(), mbi.RegionSize, &bytesRead))
            continue;

        for (size_t i = 0; i < bytesRead - patternLen; i++)
        {
            if (MatchPattern((unsigned char*)pattern, mask, buffer.data() + i))
                return addr + i;
        }
    }

    return 0;
}