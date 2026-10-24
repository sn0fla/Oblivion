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
std::atomic<bool> g_StopMemInit{ false };
uintptr_t g_spoofRets[3] = { 0, 0, 0 };

static uintptr_t g_ntdllBase = 0;
static uintptr_t g_syscallGadget = 0;
static uint8_t* g_stubPage = nullptr;
static size_t    g_stubOffset = 0;

static constexpr uint64_t kCompileSeed = []() constexpr {
    uint64_t h = 0xcbf29ce484222325ull;
    const char* s = __TIME__ __DATE__;
    for (int i = 0; s[i]; ++i) { h ^= static_cast<uint8_t>(s[i]); h *= 0x100000001b3ull; }
    return h;
    }();

static bool SafeMemcpy(uintptr_t src, void* dst, size_t size) {
    __try {
        memcpy(dst, reinterpret_cast<const void*>(src), size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static uintptr_t GetNtdllBase() {
    if (!g_ntdllBase) {
        HMODULE h = GetModuleHandleA(xorstr_("ntdll.dll"));
        g_ntdllBase = reinterpret_cast<uintptr_t>(h);
    }
    return g_ntdllBase;
}

static uintptr_t FindExportByName(uintptr_t moduleBase, const char* name) {
    if (!moduleBase || !name) return 0;
    IMAGE_DOS_HEADER dos{};
    if (!SafeMemcpy(moduleBase, &dos, sizeof(dos))) return 0;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) return 0;
    IMAGE_NT_HEADERS nt{};
    if (!SafeMemcpy(moduleBase + dos.e_lfanew, &nt, sizeof(nt))) return 0;
    if (nt.Signature != IMAGE_NT_SIGNATURE) return 0;
    DWORD expRva = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!expRva) return 0;
    IMAGE_EXPORT_DIRECTORY exp{};
    if (!SafeMemcpy(moduleBase + expRva, &exp, sizeof(exp))) return 0;
    if (exp.NumberOfNames == 0 || exp.NumberOfFunctions == 0) return 0;
    std::vector<DWORD> names(exp.NumberOfNames);
    std::vector<DWORD> funcs(exp.NumberOfFunctions);
    std::vector<WORD>  ords(exp.NumberOfNames);
    if (!SafeMemcpy(moduleBase + exp.AddressOfNames, names.data(), names.size() * sizeof(DWORD))) return 0;
    if (!SafeMemcpy(moduleBase + exp.AddressOfFunctions, funcs.data(), funcs.size() * sizeof(DWORD))) return 0;
    if (!SafeMemcpy(moduleBase + exp.AddressOfNameOrdinals, ords.data(), ords.size() * sizeof(WORD))) return 0;
    for (DWORD i = 0; i < exp.NumberOfNames; ++i) {
        char fnName[256]{};
        if (!SafeMemcpy(moduleBase + names[i], fnName, sizeof(fnName) - 1)) continue;
        if (strcmp(fnName, name) != 0) continue;
        WORD ord = ords[i];
        if (ord >= exp.NumberOfFunctions) return 0;
        return moduleBase + funcs[ord];
    }
    return 0;
}

static uint32_t ExtractSSNFromStub(uintptr_t addr) {
    BYTE buf[8]{};
    if (!SafeMemcpy(addr, buf, sizeof(buf))) return 0;
    if (buf[0] == 0x4C && buf[1] == 0x8B && buf[2] == 0xD1 && buf[3] == 0xB8)
        return *reinterpret_cast<uint32_t*>(buf + 4);
    return 0;
}

static uintptr_t ExtractGadgetFromStub(uintptr_t addr) {
    BYTE buf[64]{};
    if (!SafeMemcpy(addr, buf, sizeof(buf))) return 0;
    for (int i = 0; i + 2 < static_cast<int>(sizeof(buf)); ++i) {
        if (buf[i] == 0x0F && buf[i + 1] == 0x05 && buf[i + 2] == 0xC3)
            return addr + i;
    }
    return 0;
}

static uint32_t ResolveSSN(uintptr_t funcAddr) {
    uint32_t ssn = ExtractSSNFromStub(funcAddr);
    if (ssn) return ssn;
    for (int i = 1; i <= 64; ++i) {
        uintptr_t neighbor = funcAddr - static_cast<uintptr_t>(i) * 32;
        uint32_t nb = ExtractSSNFromStub(neighbor);
        if (nb) return nb + static_cast<uint32_t>(i);
    }
    return 0;
}

static uintptr_t FindRetInText(uintptr_t moduleBase) {
    if (!moduleBase) return 0;
    IMAGE_DOS_HEADER dos{};
    if (!SafeMemcpy(moduleBase, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE) return 0;
    IMAGE_NT_HEADERS nt{};
    if (!SafeMemcpy(moduleBase + dos.e_lfanew, &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE) return 0;
    auto* sec = IMAGE_FIRST_SECTION(reinterpret_cast<IMAGE_NT_HEADERS*>(moduleBase + dos.e_lfanew));
    for (int i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
        if (memcmp(sec[i].Name, ".text", 5) != 0) continue;
        uintptr_t start = moduleBase + sec[i].VirtualAddress;
        SIZE_T size = sec[i].Misc.VirtualSize;
        if (size < 0x100) return 0;
        std::vector<uint8_t> buf(size);
        if (!SafeMemcpy(start, buf.data(), size)) return 0;
        std::vector<SIZE_T> hits;
        for (SIZE_T j = 0; j + 2 < size; ++j) {
            if (buf[j] == 0xC3 && buf[j + 1] != 0xCC && buf[j + 1] != 0xC3)
                hits.push_back(j);
        }
        if (hits.empty()) return 0;
        SIZE_T pick = hits[Rng::Next32() % hits.size()];
        return start + pick;
    }
    return 0;
}

static void InitSpoofRets() {
    if (g_spoofRets[0] && g_spoofRets[1] && g_spoofRets[2]) return;
    HMODULE mods[3] = {
        GetModuleHandleA(xorstr_("kernel32.dll")),
        GetModuleHandleA(xorstr_("ntdll.dll")),
        GetModuleHandleA(xorstr_("kernelbase.dll"))
    };
    for (int i = 0; i < 3; ++i) {
        if (!g_spoofRets[i] && mods[i])
            g_spoofRets[i] = FindRetInText(reinterpret_cast<uintptr_t>(mods[i]));
    }
    if (!g_spoofRets[0]) g_spoofRets[0] = GetNtdllBase() ? FindRetInText(GetNtdllBase()) : 0;
    if (!g_spoofRets[1]) g_spoofRets[1] = g_spoofRets[0];
    if (!g_spoofRets[2]) g_spoofRets[2] = g_spoofRets[0];
}

static bool InitSyscallEngine() {
    if (g_syscallGadget) return true;
    uintptr_t ntdll = GetNtdllBase();
    if (!ntdll) return false;
    const char* probes[] = {
        "NtReadVirtualMemory", "NtWriteVirtualMemory", "NtOpenProcess",
        "NtClose", "NtQuerySystemInformation", "NtAllocateVirtualMemory",
        "NtProtectVirtualMemory", "NtCreateFile", "NtQueryInformationProcess"
    };
    for (const char* name : probes) {
        uintptr_t fn = FindExportByName(ntdll, name);
        if (!fn) continue;
        uintptr_t gadget = ExtractGadgetFromStub(fn);
        if (gadget) { g_syscallGadget = gadget; break; }
    }
    if (!g_syscallGadget) return false;
    g_stubPage = static_cast<uint8_t*>(
        VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!g_stubPage) return false;
    InitSpoofRets();
    return true;
}

static void* BuildStub(uint32_t ssn) {
    if (!g_stubPage || !g_syscallGadget) return nullptr;
    if (g_stubOffset + 64 > 0x1000) return nullptr;

    InitSpoofRets();

    int nFrames = 0;
    if (g_spoofRets[0]) nFrames++;
    if (g_spoofRets[1] && nFrames >= 1) nFrames++;
    if (g_spoofRets[2] && nFrames >= 2) nFrames++;
    if (nFrames < 1) nFrames = 1;

    uint8_t* s = g_stubPage + g_stubOffset;
    memset(s, 0xCC, 64);

    SIZE_T codeSize = static_cast<SIZE_T>(nFrames) * 6 + 14;
    SIZE_T dataStart = ((codeSize + 7) / 8) * 8;

    SIZE_T o = 0;
    for (int i = nFrames - 1; i >= 0; --i) {
        SIZE_T pos = o;
        s[o++] = 0xFF; s[o++] = 0x35;
        int32_t disp = static_cast<int32_t>((dataStart + 8 + static_cast<SIZE_T>(i) * 8) - (pos + 6));
        *reinterpret_cast<int32_t*>(s + o) = disp;
        o += 4;
    }

    s[o++] = 0x4C; s[o++] = 0x8B; s[o++] = 0xD1;
    s[o++] = 0xB8;
    *reinterpret_cast<uint32_t*>(s + o) = ssn;
    o += 4;
    SIZE_T jmpPos = o;
    s[o++] = 0xFF; s[o++] = 0x25;
    int32_t jmpDisp = static_cast<int32_t>(dataStart - (jmpPos + 6));
    *reinterpret_cast<int32_t*>(s + o) = jmpDisp;
    o += 4;

    while (o < dataStart) s[o++] = 0xCC;

    *reinterpret_cast<uintptr_t*>(s + dataStart + 0) = g_syscallGadget;
    for (int i = 0; i < nFrames; ++i)
        *reinterpret_cast<uintptr_t*>(s + dataStart + 8 + static_cast<SIZE_T>(i) * 8) = g_spoofRets[i];

    void* result = s;
    g_stubOffset += 64;
    return result;
}

static void* BuildStubFor(const char* name) {
    if (!InitSyscallEngine()) return nullptr;
    uintptr_t fn = FindExportByName(GetNtdllBase(), name);
    if (!fn) return nullptr;
    uint32_t ssn = ResolveSSN(fn);
    if (!ssn) return nullptr;
    return BuildStub(ssn);
}

pNtReadVirtualMemory_t GetNtReadVirtualMemory() {
    static pNtReadVirtualMemory_t fn = []() -> pNtReadVirtualMemory_t {
        void* stub = BuildStubFor(xorstr_("NtReadVirtualMemory"));
        if (stub) return reinterpret_cast<pNtReadVirtualMemory_t>(stub);
        HMODULE ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
        return reinterpret_cast<pNtReadVirtualMemory_t>(GetProcAddress(ntdll, xorstr_("NtReadVirtualMemory")));
        }();
    return fn;
}

pNtWriteVirtualMemory_t GetNtWriteVirtualMemory() {
    static pNtWriteVirtualMemory_t fn = []() -> pNtWriteVirtualMemory_t {
        void* stub = BuildStubFor(xorstr_("NtWriteVirtualMemory"));
        if (stub) return reinterpret_cast<pNtWriteVirtualMemory_t>(stub);
        HMODULE ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
        return reinterpret_cast<pNtWriteVirtualMemory_t>(GetProcAddress(ntdll, xorstr_("NtWriteVirtualMemory")));
        }();
    return fn;
}

pNtOpenProcess_t GetNtOpenProcess() {
    static pNtOpenProcess_t fn = []() -> pNtOpenProcess_t {
        void* stub = BuildStubFor(xorstr_("NtOpenProcess"));
        if (stub) return reinterpret_cast<pNtOpenProcess_t>(stub);
        HMODULE ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
        return reinterpret_cast<pNtOpenProcess_t>(GetProcAddress(ntdll, xorstr_("NtOpenProcess")));
        }();
    return fn;
}

static pNtDuplicateObject_t GetNtDuplicateObject() {
    static pNtDuplicateObject_t fn = []() -> pNtDuplicateObject_t {
        void* stub = BuildStubFor(xorstr_("NtDuplicateObject"));
        if (stub) return reinterpret_cast<pNtDuplicateObject_t>(stub);
        HMODULE ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
        return reinterpret_cast<pNtDuplicateObject_t>(GetProcAddress(ntdll, xorstr_("NtDuplicateObject")));
        }();
    return fn;
}

static pNtQuerySystemInformation_t GetNtQuerySystemInformation() {
    static pNtQuerySystemInformation_t fn = []() -> pNtQuerySystemInformation_t {
        void* stub = BuildStubFor(xorstr_("NtQuerySystemInformation"));
        if (stub) return reinterpret_cast<pNtQuerySystemInformation_t>(stub);
        HMODULE ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
        return reinterpret_cast<pNtQuerySystemInformation_t>(GetProcAddress(ntdll, xorstr_("NtQuerySystemInformation")));
        }();
    return fn;
}

typedef NTSTATUS(NTAPI* pNtQueryInformationProcess_t)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

static pNtQueryInformationProcess_t GetNtQueryInformationProcess() {
    static pNtQueryInformationProcess_t fn = []() -> pNtQueryInformationProcess_t {
        void* stub = BuildStubFor(xorstr_("NtQueryInformationProcess"));
        if (stub) return reinterpret_cast<pNtQueryInformationProcess_t>(stub);
        HMODULE ntdll = GetModuleHandleA(xorstr_("ntdll.dll"));
        return reinterpret_cast<pNtQueryInformationProcess_t>(GetProcAddress(ntdll, xorstr_("NtQueryInformationProcess")));
        }();
    return fn;
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
    NTSTATUS status = NtOpenProcessFn(&hProcess,
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, &oa, &cid);
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
        L"steam.exe", L"steamservice.exe", L"explorer.exe", L"dwm.exe",
        L"csrss.exe", L"svchost.exe", L"nvcontainer.exe", L"audiodg.exe"
    };
    for (const auto& t : trusted) if (name == t) return true;
    return false;
}

static bool IsHandleAlive(HANDLE h) {
    if (!h) return false;
    DWORD code = 0;
    if (!GetExitCodeProcess(h, &code) && GetLastError() != ERROR_INVALID_HANDLE) return true;
    return code == STILL_ACTIVE;
}

static void LaunchCS2() {
    ShellExecuteA(nullptr, "open", "steam://rungameid/730", nullptr, nullptr, SW_SHOWNORMAL);
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
        if (LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
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
        NTSTATUS dupStatus = NtDuplicate(hSourceProc, hOriginal, GetCurrentProcess(),
            &hDup, 0, 0, DUPLICATE_SAME_ACCESS);
        CloseHandle(hSourceProc);
        if (!NT_SUCCESS(dupStatus) || !hDup) continue;
        auto NtQueryInfoProc = GetNtQueryInformationProcess();
        if (NtQueryInfoProc) {
            PROCESS_BASIC_INFORMATION pbi = {};
            ULONG ret = 0;
            if (NT_SUCCESS(NtQueryInfoProc(hDup, ProcessBasicInformation, &pbi, sizeof(pbi), &ret))) {
                DWORD handlePid = (DWORD)pbi.UniqueProcessId;
                if (handlePid == targetPid) {
                    HANDLE hFinal = nullptr;
                    NTSTATUS finalStatus = NtDuplicate(GetCurrentProcess(), hDup,
                        GetCurrentProcess(), &hFinal,
                        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
                        0, 0);
                    CloseHandle(hDup);
                    if (NT_SUCCESS(finalStatus) && hFinal) return hFinal;
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
    auto NtQueryInfoProc = GetNtQueryInformationProcess();
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
        if (g_StopMemInit.load(std::memory_order_relaxed)) return false;
        LaunchCS2();
        const ULONGLONG timeoutMs = 5ull * 60ull * 1000ull;
        const ULONGLONG startTick = GetTickCount64();
        while ((GetTickCount64() - startTick) < timeoutMs) {
            Sleep(1000);
            if (g_StopMemInit.load(std::memory_order_relaxed)) return false;
            pid = FindProcessId(processName);
            if (pid) break;
        }
        if (!pid) {
            MessageBoxA(NULL, "Timed out waiting for 'cs2.exe' to launch (5 minutes).",
                "Memory Init Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    if (!IsHandleAlive(handle)) {
        if (handle) { CloseHandle(handle); handle = nullptr; }
        handle = TryHijackHandle(pid);
    }
    if (!handle) {
        if (g_StopMemInit.load(std::memory_order_relaxed)) return false;
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
        const ULONGLONG timeoutMs = 5ull * 60ull * 1000ull;
        const ULONGLONG startTick = GetTickCount64();
        while ((GetTickCount64() - startTick) < timeoutMs) {
            Sleep(1000);
            if (g_StopMemInit.load(std::memory_order_relaxed)) return false;
            client = GetModuleBase(clientDll.c_str());
            engine = GetModuleBase(engineDll.c_str());
            if (client && engine) break;
        }
        if (!client || !engine) {
            MessageBoxA(NULL, "Timed out waiting for client.dll / engine2.dll to load (5 minutes).",
                "Memory Init Error", MB_OK | MB_ICONERROR);
            Cleanup();
            return false;
        }
    }
    return true;
}

Memory mem;