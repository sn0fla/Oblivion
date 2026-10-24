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
extern std::atomic<bool> g_StopMemInit;
extern uintptr_t         g_spoofRets[3];

namespace Rng {
    inline uint64_t Next() {
        static uint64_t s = [] {
            uint64_t v = 0xcbf29ce484222325ull;
            const char* t = __TIME__ __DATE__;
            for (int i = 0; t[i]; ++i) { v ^= static_cast<uint8_t>(t[i]); v *= 0x100000001b3ull; }
            v ^= GetTickCount64();
            v ^= static_cast<uint64_t>(GetCurrentProcessId()) << 32;
            v ^= reinterpret_cast<uintptr_t>(&v);
            LARGE_INTEGER pc; QueryPerformanceCounter(&pc);
            v ^= static_cast<uint64_t>(pc.QuadPart);
            v ^= static_cast<uint64_t>(GetCurrentThreadId()) << 16;
            return v ? v : 0x9e3779b97f4a7c15ull;
            }();
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        return s;
    }
    inline uint32_t Next32() { return static_cast<uint32_t>(Next()); }
    inline int Int(int lo, int hi) {
        if (lo >= hi) return lo;
        return lo + static_cast<int>(Next32() % static_cast<uint32_t>(hi - lo + 1));
    }
    inline std::string Str(int len, bool digits = true) {
        static const char a[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        static const char b[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        const char* s = digits ? a : b;
        const int n = digits ? 62 : 52;
        std::string r;
        r.reserve(len);
        for (int i = 0; i < len; ++i) r.push_back(s[Next32() % n]);
        return r;
    }
    inline std::wstring WStr(int len, bool digits = true) {
        std::string s = Str(len, digits);
        return std::wstring(s.begin(), s.end());
    }
}

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