#include "Memory.hpp"
#include "xorstr.hpp"
#include <winternl.h>
#include <vector>
#include <algorithm>
#include <string>
#include <cwctype>
#include <securitybaseapi.h>
#include <shellapi.h>

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "shell32.lib")

std::atomic<bool> g_AllowMemoryWrite{ false };

static uintptr_t FindSyscallStub(const char* funcName) {
    HMODULE ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
    if (!ntdll) return 0;

    FARPROC proc = GetProcAddress(ntdll, funcName);
    if (!proc) return 0;

    BYTE* bytes = (BYTE*)proc;
    for (int i = 0; i < 0x60; i++) {
        if (bytes[i] == 0x4C && bytes[i + 1] == 0x8B && bytes[i + 2] == 0xD1 && bytes[i + 3] == 0xB8) {
            return (uintptr_t)(bytes + i);
        }
    }
    return 0;
}

pNtReadVirtualMemory_t GetNtReadVirtualMemory() {
    static auto pFn = (pNtReadVirtualMemory_t)FindSyscallStub(xorstr_("NtReadVirtualMemory"));
    return pFn;
}

pNtWriteVirtualMemory_t GetNtWriteVirtualMemory() {
    static auto pFn = (pNtWriteVirtualMemory_t)FindSyscallStub(xorstr_("NtWriteVirtualMemory"));
    return pFn;
}

pNtOpenProcess_t GetNtOpenProcess() {
    static auto pFn = (pNtOpenProcess_t)FindSyscallStub(xorstr_("NtOpenProcess"));
    return pFn;
}

static pNtDuplicateObject_t GetNtDuplicateObject() {
    static auto pFn = (pNtDuplicateObject_t)FindSyscallStub(xorstr_("NtDuplicateObject"));
    return pFn;
}

static pNtQuerySystemInformation_t GetNtQuerySystemInformation() {
    static auto pFn = (pNtQuerySystemInformation_t)FindSyscallStub(xorstr_("NtQuerySystemInformation"));
    return pFn;
}

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO {
    USHORT UniqueProcessId;
    USHORT CreatorBackTraceIndex;
    UCHAR  ObjectTypeIndex;
    UCHAR  HandleAttributes;
    USHORT HandleValue;
    PVOID  Object;
    ULONG  GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION {
    ULONG NumberOfHandles;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;

typedef struct _MY_SYSTEM_PROCESS_INFORMATION {
    ULONG          NextEntryOffset;
    ULONG          NumberOfThreads;
    LARGE_INTEGER  SpareLi1;
    LARGE_INTEGER  SpareLi2;
    LARGE_INTEGER  SpareLi3;
    LARGE_INTEGER  CreateTime;
    LARGE_INTEGER  UserTime;
    LARGE_INTEGER  KernelTime;
    UNICODE_STRING ImageName;
    ULONG          BasePriority;
    HANDLE         UniqueProcessId;
    HANDLE         InheritedFromUniqueProcessId;
    ULONG          HandleCount;
    ULONG          SessionId;
    ULONG_PTR      UniqueProcessKey;
    SIZE_T         PeakVirtualSize;
    SIZE_T         VirtualSize;
    ULONG          PageFaultCount;
    SIZE_T         PeakWorkingSetSize;
    SIZE_T         WorkingSetSize;
    SIZE_T         QuotaPeakPagedPoolUsage;
    SIZE_T         QuotaPagedPoolUsage;
    SIZE_T         QuotaPeakNonPagedPoolUsage;
    SIZE_T         QuotaNonPagedPoolUsage;
    SIZE_T         PagefileUsage;
    SIZE_T         PeakPagefileUsage;
    SIZE_T         PrivatePageCount;
    LARGE_INTEGER  ReadOperationCount;
    LARGE_INTEGER  WriteOperationCount;
    LARGE_INTEGER  OtherOperationCount;
    LARGE_INTEGER  ReadTransferCount;
    LARGE_INTEGER  WriteTransferCount;
    LARGE_INTEGER  OtherTransferCount;
} MY_SYSTEM_PROCESS_INFORMATION, * PMY_SYSTEM_PROCESS_INFORMATION;

static uint32_t FindProcessId(const wchar_t* processName) {
    auto NtQuerySystemInfo = GetNtQuerySystemInformation();
    if (!NtQuerySystemInfo) return 0;

    ULONG bufferSize = 0x10000;
    std::vector<BYTE> buffer(bufferSize);
    NTSTATUS status;
    while ((status = NtQuerySystemInfo(SystemProcessInformation, buffer.data(), bufferSize, &bufferSize))
        == (NTSTATUS)0xC0000004L) {
        buffer.resize(bufferSize);
    }

    if (!NT_SUCCESS(status)) return 0;

    auto* entry = (MY_SYSTEM_PROCESS_INFORMATION*)buffer.data();
    for (;;) {
        if (entry->ImageName.Buffer &&
            _wcsicmp(entry->ImageName.Buffer, processName) == 0) {
            return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(entry->UniqueProcessId));
        }
        if (!entry->NextEntryOffset) break;
        entry = (MY_SYSTEM_PROCESS_INFORMATION*)((BYTE*)entry + entry->NextEntryOffset);
    }
    return 0;
}

static HANDLE OpenGameProcess(DWORD pid) {
    auto NtOpenProcessFn = GetNtOpenProcess();
    if (!NtOpenProcessFn) return nullptr;

    HANDLE hProcess = nullptr;
    OBJECT_ATTRIBUTES oa = { sizeof(OBJECT_ATTRIBUTES) };
    CLIENT_ID cid = { (HANDLE)(uintptr_t)pid, nullptr };

    NTSTATUS status = NtOpenProcessFn(
        &hProcess,
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION,
        &oa,
        &cid
    );
    return NT_SUCCESS(status) ? hProcess : nullptr;
}

static bool ReadRemoteBuffer(HANDLE hProcess, uintptr_t address, void* buffer, SIZE_T size) {
    auto NtRead = GetNtReadVirtualMemory();
    if (!NtRead) return false;

    SIZE_T bytesRead = 0;
    NTSTATUS status = NtRead(hProcess, (PVOID)address, buffer, size, &bytesRead);
    return NT_SUCCESS(status) && bytesRead == size;
}

std::wstring Memory::GetProcessNameByPid(DWORD pid) {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) return L"";

    WCHAR name[MAX_PATH] = { 0 };
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(hProc, 0, name, &size)) {
        std::wstring full(name);
        size_t pos = full.find_last_of(L'\\');
        if (pos != std::wstring::npos) full = full.substr(pos + 1);
        CloseHandle(hProc);
        return full;
    }
    CloseHandle(hProc);
    return L"";
}

bool Memory::IsTrustedSourceProcess(DWORD pid) {
    std::wstring name = GetProcessNameByPid(pid);
    if (name.empty()) return false;

    std::transform(name.begin(), name.end(), name.begin(), ::towlower);

    static const wchar_t* trusted[] = {
        L"steam.exe",       L"steamservice.exe",
        L"explorer.exe",    L"dwm.exe",
        L"csrss.exe",       L"svchost.exe",
        L"nvcontainer.exe", L"audiodg.exe"
    };

    for (const auto& t : trusted) {
        if (name == t) return true;
    }
    return false;
}

static bool IsHandleAlive(HANDLE h) {
    if (!h) return false;
    DWORD code = 0;
    if (!GetExitCodeProcess(h, &code) && GetLastError() != ERROR_INVALID_HANDLE)
        return true;
    return code == STILL_ACTIVE;
}

static void LaunchCS2() {
    ShellExecuteA(nullptr, "open", "steam://rungameid/730",
        nullptr, nullptr, SW_SHOWNORMAL);
}

HANDLE Memory::TryHijackHandle(DWORD targetPid) {
    auto NtQuerySystemInfo = GetNtQuerySystemInformation();
    auto NtDuplicate = GetNtDuplicateObject();
    if (!NtQuerySystemInfo || !NtDuplicate) return nullptr;

    HANDLE hToken = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        TOKEN_PRIVILEGES tp = {};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
        }
        CloseHandle(hToken);
    }

    ULONG bufferSize = 0x100000;
    std::vector<BYTE> buffer(bufferSize);
    ULONG returnLen = 0;
    NTSTATUS status = NtQuerySystemInfo((SYSTEM_INFORMATION_CLASS)0x10, buffer.data(), bufferSize, &returnLen);
    while (status == (NTSTATUS)0xC0000004L) {
        bufferSize = returnLen + 0x1000;
        buffer.resize(bufferSize);
        status = NtQuerySystemInfo((SYSTEM_INFORMATION_CLASS)0x10, buffer.data(), bufferSize, &returnLen);
    }
    if (!NT_SUCCESS(status)) return nullptr;

    auto* handleInfo = (PSYSTEM_HANDLE_INFORMATION)buffer.data();

    for (ULONG i = 0; i < handleInfo->NumberOfHandles; i++) {
        auto& entry = handleInfo->Handles[i];
        if (entry.HandleValue == 0) continue;

        DWORD sourcePid = entry.UniqueProcessId;
        if (!IsTrustedSourceProcess(sourcePid)) continue;

        HANDLE hSourceProc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, sourcePid);
        if (!hSourceProc) continue;

        HANDLE hDup = nullptr;
        HANDLE hOriginal = (HANDLE)(uintptr_t)entry.HandleValue;

        NTSTATUS dupStatus = NtDuplicate(
            hSourceProc,
            hOriginal,
            GetCurrentProcess(),
            &hDup,
            0,
            0,
            DUPLICATE_SAME_ACCESS
        );
        CloseHandle(hSourceProc);

        if (!NT_SUCCESS(dupStatus) || !hDup) continue;

        typedef NTSTATUS(NTAPI* pNtQueryInfoProc)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
        static auto NtQueryInfoProc = (pNtQueryInfoProc)FindSyscallStub(xorstr_("NtQueryInformationProcess"));
        if (NtQueryInfoProc) {
            PROCESS_BASIC_INFORMATION pbi = {};
            ULONG ret = 0;
            if (NT_SUCCESS(NtQueryInfoProc(hDup, ProcessBasicInformation, &pbi, sizeof(pbi), &ret))) {
                DWORD handlePid = (DWORD)pbi.UniqueProcessId;
                if (handlePid == targetPid) {
                    HANDLE hFinal = nullptr;
                    NTSTATUS finalStatus = NtDuplicate(
                        GetCurrentProcess(),
                        hDup,
                        GetCurrentProcess(),
                        &hFinal,
                        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
                        0,
                        0
                    );
                    CloseHandle(hDup);
                    if (NT_SUCCESS(finalStatus) && hFinal) {
                        return hFinal;
                    }
                }
            }
        }
        CloseHandle(hDup);
    }
    return nullptr;
}

bool Memory::ReadRaw(uintptr_t address, void* buffer, size_t size) const {
    if (!address || !buffer || !handle || !readFn) return false;

    SIZE_T bytesRead = 0;
    NTSTATUS status = readFn(handle, (PVOID)address, buffer, size, &bytesRead);
    return NT_SUCCESS(status) && bytesRead == size;
}

void Memory::Cleanup() {
    if (handle) {
        HMODULE ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
        if (ntdll) {
            typedef NTSTATUS(NTAPI* pNtClose)(HANDLE);
            auto NtClose = (pNtClose)GetProcAddress(ntdll, xorstr_("NtClose"));
            if (NtClose) NtClose(handle);
        }
        handle = nullptr;
    }
    pid = 0; client = 0; engine = 0;
    readFn = nullptr;
    writeFn = nullptr;
}

uint32_t Memory::GetProcessId(const wchar_t* processName) const {
    return FindProcessId(processName);
}

typedef struct _LDR_DATA_TABLE_ENTRY_FULL {
    LIST_ENTRY     InLoadOrderLinks;
    LIST_ENTRY     InMemoryOrderLinks;
    LIST_ENTRY     InInitializationOrderLinks;
    PVOID          DllBase;
    PVOID          EntryPoint;
    ULONG          SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
} LDR_DATA_TABLE_ENTRY_FULL, * PLDR_DATA_TABLE_ENTRY_FULL;

uintptr_t Memory::GetModuleBase(const wchar_t* moduleName) const {
    if (!handle) return 0;

    typedef NTSTATUS(NTAPI* pNtQueryInfoProc)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
    static auto NtQueryInfoProc = (pNtQueryInfoProc)FindSyscallStub(xorstr_("NtQueryInformationProcess"));
    if (!NtQueryInfoProc) return 0;

    PROCESS_BASIC_INFORMATION pbi{};
    ULONG returnLength = 0;
    NTSTATUS status = NtQueryInfoProc(handle, ProcessBasicInformation, &pbi, sizeof(pbi), &returnLength);
    if (!NT_SUCCESS(status) || !pbi.PebBaseAddress) return 0;

    PEB peb{};
    if (!ReadRemoteBuffer(handle, (uintptr_t)pbi.PebBaseAddress, &peb, sizeof(peb))) return 0;
    if (!peb.Ldr) return 0;

    PEB_LDR_DATA ldr{};
    if (!ReadRemoteBuffer(handle, (uintptr_t)peb.Ldr, &ldr, sizeof(ldr))) return 0;

    uintptr_t headAddr = (uintptr_t)peb.Ldr + offsetof(PEB_LDR_DATA, InMemoryOrderModuleList);
    uintptr_t currentAddr = (uintptr_t)ldr.InMemoryOrderModuleList.Flink;

    while (currentAddr && currentAddr != headAddr) {
        uintptr_t entryBase = currentAddr - offsetof(LDR_DATA_TABLE_ENTRY_FULL, InMemoryOrderLinks);
        LDR_DATA_TABLE_ENTRY_FULL entry{};
        if (!ReadRemoteBuffer(handle, entryBase, &entry, sizeof(entry))) break;

        if (entry.BaseDllName.Buffer && entry.BaseDllName.Length > 0) {
            WCHAR nameBuffer[MAX_PATH]{};
            SIZE_T nameSize = (std::min)(
                (SIZE_T)entry.BaseDllName.Length,
                (SIZE_T)(MAX_PATH - 1) * sizeof(WCHAR));
            if (ReadRemoteBuffer(handle, (uintptr_t)entry.BaseDllName.Buffer, nameBuffer, nameSize)) {
                if (_wcsicmp(nameBuffer, moduleName) == 0)
                    return (uintptr_t)entry.DllBase;
            }
        }
        currentAddr = (uintptr_t)entry.InMemoryOrderLinks.Flink;
    }
    return 0;
}

bool Memory::Init(const wchar_t* processName) {
    readFn = GetNtReadVirtualMemory();
    writeFn = GetNtWriteVirtualMemory();

    pid = FindProcessId(processName);

    if (!pid) {
        LaunchCS2();

        const ULONGLONG timeoutMs = 5ull * 60ull * 1000ull;
        const ULONGLONG startTick = GetTickCount64();

        while ((GetTickCount64() - startTick) < timeoutMs) {
            Sleep(1000);
            pid = FindProcessId(processName);
            if (pid) break;
        }

        if (!pid) {
            MessageBoxA(NULL,
                "Timed out waiting for 'cs2.exe' to launch (5 minutes).",
                "Memory Init Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    if (!IsHandleAlive(handle)) {
        if (handle) { CloseHandle(handle); handle = nullptr; }
        handle = TryHijackHandle(pid);
    }

    if (!handle) {
        handle = OpenGameProcess(pid);
        if (!handle) {
            MessageBoxA(NULL, "Failed to open process handle (both hijack and fallback).",
                "Memory Init Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    std::wstring clientDll = xorstr_(L"client.dll");
    std::wstring engineDll = xorstr_(L"engine2.dll");

    client = GetModuleBase(clientDll.c_str());
    engine = GetModuleBase(engineDll.c_str());

    if (!client || !engine) {
        MessageBoxA(NULL, "Failed to get module bases (client.dll / engine2.dll).",
            "Memory Init Error", MB_OK | MB_ICONERROR);
        Cleanup();
        return false;
    }

    return true;
}

Memory mem;