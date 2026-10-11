#include "Protection.hpp"
#include "../../External/Xorstr/xorstr.hpp"
#include <psapi.h>
#include <tlhelp32.h>
#include <intrin.h>
#include <vector>

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "psapi.lib")

static pNtReadVirtualMemory_t       g_NtReadVirtualMemoryStub = nullptr;
static pNtWriteVirtualMemory_t      g_NtWriteVirtualMemoryStub = nullptr;
static pNtOpenProcess_t             g_NtOpenProcessStub = nullptr;
static pNtDuplicateObject_t         g_NtDuplicateObjectStub = nullptr;
static pNtQuerySystemInformation_t  g_NtQuerySystemInformationStub = nullptr;
static pNtQueryInformationProcess_t g_NtQueryInformationProcessStub = nullptr;

static uintptr_t FindSyscallStub(const char* funcName) {
    HMODULE ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
    if (!ntdll) return 0;

    uintptr_t funcAddr = (uintptr_t)GetProcAddress(ntdll, funcName);
    if (!funcAddr) return 0;

    BYTE* bytes = (BYTE*)funcAddr;
    for (int i = 0; i < 0x60; i++) {
        if (bytes[i] == 0x4C && bytes[i + 1] == 0x8B && bytes[i + 2] == 0xD1 && bytes[i + 3] == 0xB8)
            return (uintptr_t)(bytes + i);
    }
    return 0;
}

static void InitSyscallStubs() {
    if (!g_NtReadVirtualMemoryStub)
        g_NtReadVirtualMemoryStub = (pNtReadVirtualMemory_t)FindSyscallStub(xorstr_("NtReadVirtualMemory"));
    if (!g_NtWriteVirtualMemoryStub)
        g_NtWriteVirtualMemoryStub = (pNtWriteVirtualMemory_t)FindSyscallStub(xorstr_("NtWriteVirtualMemory"));
    if (!g_NtOpenProcessStub)
        g_NtOpenProcessStub = (pNtOpenProcess_t)FindSyscallStub(xorstr_("NtOpenProcess"));
    if (!g_NtDuplicateObjectStub)
        g_NtDuplicateObjectStub = (pNtDuplicateObject_t)FindSyscallStub(xorstr_("NtDuplicateObject"));
    if (!g_NtQuerySystemInformationStub)
        g_NtQuerySystemInformationStub = (pNtQuerySystemInformation_t)FindSyscallStub(xorstr_("NtQuerySystemInformation"));
    if (!g_NtQueryInformationProcessStub)
        g_NtQueryInformationProcessStub = (pNtQueryInformationProcess_t)FindSyscallStub(xorstr_("NtQueryInformationProcess"));
}

pNtReadVirtualMemory_t GetNtReadVirtualMemoryStub() {
    if (!g_NtReadVirtualMemoryStub) InitSyscallStubs();
    return g_NtReadVirtualMemoryStub;
}
pNtWriteVirtualMemory_t GetNtWriteVirtualMemoryStub() {
    if (!g_NtWriteVirtualMemoryStub) InitSyscallStubs();
    return g_NtWriteVirtualMemoryStub;
}
pNtOpenProcess_t GetNtOpenProcessStub() {
    if (!g_NtOpenProcessStub) InitSyscallStubs();
    return g_NtOpenProcessStub;
}
pNtDuplicateObject_t GetNtDuplicateObjectStub() {
    if (!g_NtDuplicateObjectStub) InitSyscallStubs();
    return g_NtDuplicateObjectStub;
}
pNtQuerySystemInformation_t GetNtQuerySystemInformationStub() {
    if (!g_NtQuerySystemInformationStub) InitSyscallStubs();
    return g_NtQuerySystemInformationStub;
}
pNtQueryInformationProcess_t GetNtQueryInformationProcessStub() {
    if (!g_NtQueryInformationProcessStub) InitSyscallStubs();
    return g_NtQueryInformationProcessStub;
}

bool IsDebuggerPresentCheck() {
    return false;
}

void AntiDebugInit() {
}

void Protection::Report(const char* title, const char* description) {
    const char* t = title ? title : "ERROR";
    const char* d = description ? description : "";
    MessageBoxA(nullptr, d, t, MB_OK | MB_ICONERROR);
}