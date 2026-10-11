#pragma once
#include <Windows.h>
#include <winternl.h>
#include <string>
#include <cstdint>

typedef NTSTATUS(NTAPI* pNtReadVirtualMemory_t)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS(NTAPI* pNtWriteVirtualMemory_t)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
typedef NTSTATUS(NTAPI* pNtOpenProcess_t)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, CLIENT_ID*);
typedef NTSTATUS(NTAPI* pNtDuplicateObject_t)(HANDLE, HANDLE, HANDLE, PHANDLE, ACCESS_MASK, ULONG, ULONG);
typedef NTSTATUS(NTAPI* pNtQuerySystemInformation_t)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS(NTAPI* pNtQueryInformationProcess_t)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

pNtReadVirtualMemory_t        GetNtReadVirtualMemoryStub();
pNtWriteVirtualMemory_t       GetNtWriteVirtualMemoryStub();
pNtOpenProcess_t              GetNtOpenProcessStub();
pNtDuplicateObject_t          GetNtDuplicateObjectStub();
pNtQuerySystemInformation_t   GetNtQuerySystemInformationStub();
pNtQueryInformationProcess_t  GetNtQueryInformationProcessStub();

bool IsDebuggerPresentCheck();
void AntiDebugInit();

namespace Protection {
    void Report(const char* title, const char* description);
}

inline void SetDPIAware() {
    SetProcessDPIAware();
}