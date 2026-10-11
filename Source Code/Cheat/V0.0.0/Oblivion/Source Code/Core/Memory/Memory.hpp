#pragma once
#include <Windows.h>
#include <winternl.h>
#include <cstdint>
#include <string>
#include <cstring>
#include <atomic>

typedef NTSTATUS(NTAPI* pNtReadVirtualMemory_t) (HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS(NTAPI* pNtWriteVirtualMemory_t)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS(NTAPI* pNtOpenProcess_t)       (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, CLIENT_ID*);
typedef NTSTATUS(NTAPI* pNtDuplicateObject_t)   (HANDLE, HANDLE, HANDLE, PHANDLE, ACCESS_MASK, ULONG, ULONG);
typedef NTSTATUS(NTAPI* pNtQuerySystemInformation_t)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

pNtReadVirtualMemory_t  GetNtReadVirtualMemory();
pNtWriteVirtualMemory_t GetNtWriteVirtualMemory();
pNtOpenProcess_t        GetNtOpenProcess();

extern std::atomic<bool> g_AllowMemoryWrite;

class Memory {
public:
    uint32_t  pid = 0;
    HANDLE    handle = nullptr;
    uintptr_t client = 0;
    uintptr_t engine = 0;

    pNtReadVirtualMemory_t  readFn = nullptr;
    pNtWriteVirtualMemory_t writeFn = nullptr;

    bool Init(const wchar_t* processName);

    void Cleanup();
    ~Memory() { Cleanup(); }

    template <typename T>
    T Read(uintptr_t address) const {
        T buffer{};
        if (address && handle && readFn) {
            SIZE_T bytesRead = 0;
            readFn(handle, (PVOID)address, &buffer, sizeof(T), &bytesRead);
        }
        return buffer;
    }

    bool ReadRaw(uintptr_t address, void* buffer, size_t size) const;

    template <typename T>
    bool Write(uintptr_t address, T value) const {
        if (!address || !handle || !writeFn) return false;
        if (!g_AllowMemoryWrite.load(std::memory_order_relaxed)) return false;

        T currentValue = Read<T>(address);
        if (std::memcmp(&currentValue, &value, sizeof(T)) == 0)
            return true;

        SIZE_T bytesWritten = 0;
        NTSTATUS status = writeFn(handle, (PVOID)address, &value, sizeof(T), &bytesWritten);
        return NT_SUCCESS(status) && bytesWritten == sizeof(T);
    }

    uintptr_t GetModuleBase(const wchar_t* moduleName) const;

private:
    uint32_t GetProcessId(const wchar_t* processName) const;

    HANDLE       TryHijackHandle(DWORD targetPid);
    bool         IsTrustedSourceProcess(DWORD pid);
    std::wstring GetProcessNameByPid(DWORD pid);
};

extern Memory mem;