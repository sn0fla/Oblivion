#include "Logging.hpp"
#include <Windows.h>
#include <DbgHelp.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <exception>
#include <mutex>
#include <string>
#include <unordered_map>
#include <atomic>

#pragma comment(lib, "dbghelp.lib")

namespace Logging {

    struct Bucket {
        double   sum = 0.0;
        double   mn = 1e18;
        double   mx = -1e18;
        double   last = 0.0;
        uint64_t n = 0;
    };

    static std::mutex          g_mtx;
    static FILE* g_file = nullptr;
    static std::string         g_path;
    static std::atomic<bool>   g_init{ false };
    static std::atomic<double> g_start{ 0.0 };
    static std::atomic<double> g_lastFlush{ 0.0 };
    static std::unordered_map<std::string, Bucket> g_buckets;

    static double NowSec() {
        static const LARGE_INTEGER f = [] {
            LARGE_INTEGER x; QueryPerformanceFrequency(&x); return x;
            }();
        LARGE_INTEGER t; QueryPerformanceCounter(&t);
        return static_cast<double>(t.QuadPart) / static_cast<double>(f.QuadPart);
    }

    static void Stamp(char* out, size_t n) {
        SYSTEMTIME s; GetLocalTime(&s);
        snprintf(out, n, "%02u:%02u:%02u.%03u",
            s.wHour, s.wMinute, s.wSecond, s.wMilliseconds);
    }

    static void PrepareLogDir() {
        CreateDirectoryA("C:\\Oblivion", nullptr);
        CreateDirectoryA("C:\\Oblivion\\Logs", nullptr);

        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA("C:\\Oblivion\\Logs\\*", &fd);
        if (hFind == INVALID_HANDLE_VALUE) return;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            char fullPath[MAX_PATH * 2];
            snprintf(fullPath, sizeof(fullPath), "C:\\Oblivion\\Logs\\%s", fd.cFileName);
            DeleteFileA(fullPath);
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }

    static void VWrite(const char* lvl, const char* fmt, va_list ap) {
        if (!g_init.load(std::memory_order_relaxed)) return;
        std::lock_guard<std::mutex> lk(g_mtx);
        if (!g_file) return;
        char ts[24]; Stamp(ts, sizeof(ts));
        fprintf(g_file, "[%s][%s] ", ts, lvl);
        vfprintf(g_file, fmt, ap);
        fputc('\n', g_file);
        fflush(g_file);
    }

    void Info(const char* f, ...) { va_list a; va_start(a, f); VWrite("INFO", f, a); va_end(a); }
    void Warn(const char* f, ...) { va_list a; va_start(a, f); VWrite("WARN", f, a); va_end(a); }
    void Error(const char* f, ...) { va_list a; va_start(a, f); VWrite("ERROR", f, a); va_end(a); }
    void Debug(const char* f, ...) { va_list a; va_start(a, f); VWrite("DEBUG", f, a); va_end(a); }

    void Sample(const char* name, double v) {
        if (!g_init.load(std::memory_order_relaxed)) return;
        std::lock_guard<std::mutex> lk(g_mtx);
        Bucket& b = g_buckets[name];
        b.sum += v;
        if (v < b.mn) b.mn = v;
        if (v > b.mx) b.mx = v;
        b.last = v;
        b.n++;
    }

    void Tick() {
        if (!g_init.load(std::memory_order_relaxed)) return;
        const double now = NowSec();
        if (now - g_lastFlush.load(std::memory_order_relaxed) < 1.0) return;
        std::lock_guard<std::mutex> lk(g_mtx);
        if (!g_file) return;
        char ts[24]; Stamp(ts, sizeof(ts));
        fprintf(g_file, "[%s][LAT] uptime=%.1fs\n", ts,
            now - g_start.load(std::memory_order_relaxed));
        for (auto& kv : g_buckets) {
            const Bucket& b = kv.second;
            if (!b.n) continue;
            fprintf(g_file, "  %-26s n=%-6llu last=%7.2f avg=%7.2f min=%7.2f max=%7.2f\n",
                kv.first.c_str(),
                static_cast<unsigned long long>(b.n),
                b.last, b.sum / static_cast<double>(b.n), b.mn, b.mx);
            kv.second = Bucket{};
        }
        fflush(g_file);
        g_lastFlush.store(now, std::memory_order_relaxed);
    }

    static void TerminateHandler() {
        if (g_file) {
            char ts[24]; Stamp(ts, sizeof(ts));
            fprintf(g_file, "\n[%s][CRASH] std::terminate invoked\n", ts);
            fflush(g_file);
        }
        std::abort();
    }

    LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep) {
        static std::atomic<bool> inHandler{ false };
        if (inHandler.exchange(true)) return EXCEPTION_EXECUTE_HANDLER;
        FILE* f = g_file;
        if (!f) return EXCEPTION_EXECUTE_HANDLER;
        char ts[24]; Stamp(ts, sizeof(ts));
        fprintf(f, "\n[%s][CRASH] =============== UNHANDLED EXCEPTION ===============\n", ts);
        fprintf(f, "[CRASH] PID=%lu TID=%lu uptime=%.1fs\n",
            GetCurrentProcessId(), GetCurrentThreadId(),
            NowSec() - g_start.load(std::memory_order_relaxed));
        if (ep && ep->ExceptionRecord) {
            auto* er = ep->ExceptionRecord;
            fprintf(f, "[CRASH] Code:    0x%08X\n", er->ExceptionCode);
            fprintf(f, "[CRASH] Address: %p\n", er->ExceptionAddress);
            fprintf(f, "[CRASH] Flags:   0x%08X\n", er->ExceptionFlags);
            fprintf(f, "[CRASH] Params:  %llu\n",
                static_cast<unsigned long long>(er->NumberParameters));
            for (DWORD i = 0; i < er->NumberParameters && i < 15; ++i)
                fprintf(f, "  [%u] 0x%016llX\n", i,
                    static_cast<unsigned long long>(er->ExceptionInformation[i]));
            HMODULE mod = nullptr;
            if (GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(er->ExceptionAddress), &mod) && mod) {
                char p[MAX_PATH]{}; GetModuleFileNameA(mod, p, MAX_PATH);
                fprintf(f, "[CRASH] Module:  %s + 0x%llX\n", p,
                    static_cast<unsigned long long>(
                        reinterpret_cast<uintptr_t>(er->ExceptionAddress) -
                        reinterpret_cast<uintptr_t>(mod)));
            }
        }
        if (ep && ep->ContextRecord) {
            auto* c = ep->ContextRecord;
#if defined(_M_X64)
            fprintf(f, "[CRASH] RIP=%016llX RSP=%016llX RBP=%016llX\n", c->Rip, c->Rsp, c->Rbp);
            fprintf(f, "[CRASH] RAX=%016llX RBX=%016llX RCX=%016llX RDX=%016llX\n", c->Rax, c->Rbx, c->Rcx, c->Rdx);
            fprintf(f, "[CRASH] RSI=%016llX RDI=%016llX R8 =%016llX R9 =%016llX\n", c->Rsi, c->Rdi, c->R8, c->R9);
            fprintf(f, "[CRASH] R10=%016llX R11=%016llX R12=%016llX R13=%016llX\n", c->R10, c->R11, c->R12, c->R13);
            fprintf(f, "[CRASH] R14=%016llX R15=%016llX\n", c->R14, c->R15);
#endif
        }
        if (ep && ep->ContextRecord) {
            HANDLE proc = GetCurrentProcess();
            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
            if (SymInitialize(proc, nullptr, TRUE)) {
                CONTEXT ctx = *ep->ContextRecord;
                STACKFRAME64 sf{};
#if defined(_M_X64)
                sf.AddrPC.Offset = ctx.Rip;    sf.AddrPC.Mode = AddrModeFlat;
                sf.AddrStack.Offset = ctx.Rsp; sf.AddrStack.Mode = AddrModeFlat;
                sf.AddrFrame.Offset = ctx.Rbp; sf.AddrFrame.Mode = AddrModeFlat;
                const DWORD machine = IMAGE_FILE_MACHINE_AMD64;
#endif
                fprintf(f, "[CRASH] Stack:\n");
                for (int i = 0; i < 48; ++i) {
                    if (!StackWalk64(machine, proc, GetCurrentThread(), &sf, &ctx,
                        nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
                        break;
                    if (!sf.AddrPC.Offset) break;
                    DWORD64 disp = 0;
                    char buf[sizeof(SYMBOL_INFO) + 256]{};
                    auto* sym = reinterpret_cast<SYMBOL_INFO*>(buf);
                    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
                    sym->MaxNameLen = 255;
                    const char* name = "?";
                    if (SymFromAddr(proc, sf.AddrPC.Offset, &disp, sym)) name = sym->Name;
                    fprintf(f, "  #%02d %016llX %s+0x%llX\n", i,
                        static_cast<unsigned long long>(sf.AddrPC.Offset),
                        name, static_cast<unsigned long long>(disp));
                }
                SymCleanup(proc);
            }
        }
        if (ep) {
            char dumpPath[MAX_PATH + 16];
            snprintf(dumpPath, sizeof(dumpPath), "%s.dmp", g_path.c_str());
            HANDLE h = CreateFileA(dumpPath, GENERIC_WRITE, 0, nullptr,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (h != INVALID_HANDLE_VALUE) {
                MINIDUMP_EXCEPTION_INFORMATION mei{};
                mei.ThreadId = GetCurrentThreadId();
                mei.ExceptionPointers = ep;
                mei.ClientPointers = FALSE;
                const MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
                    MiniDumpWithHandleData | MiniDumpWithThreadInfo | MiniDumpWithFullMemoryInfo);
                MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), h,
                    dumpType, &mei, nullptr, nullptr);
                CloseHandle(h);
                fprintf(f, "[CRASH] Minidump: %s\n", dumpPath);
            }
        }
        fprintf(f, "[CRASH] ======================================================\n");
        fflush(f);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    void InstallCrashHandler() {
        SetUnhandledExceptionFilter(CrashHandler);
        std::set_terminate(TerminateHandler);
    }

    bool Init() {
        if (g_init.load(std::memory_order_relaxed)) return true;
        std::lock_guard<std::mutex> lk(g_mtx);

        PrepareLogDir();

        SYSTEMTIME s; GetLocalTime(&s);
        char name[MAX_PATH * 2];
        snprintf(name, sizeof(name),
            "C:\\Oblivion\\Logs\\log_%04u%02u%02u_%02u%02u%02u.log",
            s.wYear, s.wMonth, s.wDay, s.wHour, s.wMinute, s.wSecond);
        g_path = name;

        if (fopen_s(&g_file, g_path.c_str(), "w") != 0 || !g_file) {
            g_file = nullptr;
            return false;
        }

        g_start.store(NowSec(), std::memory_order_relaxed);
        g_lastFlush.store(g_start.load(), std::memory_order_relaxed);
        g_init.store(true, std::memory_order_relaxed);

        char ts[24]; Stamp(ts, sizeof(ts));
        fprintf(g_file, "[%s][INFO] Log opened: %s\n", ts, g_path.c_str());
        fprintf(g_file, "[%s][INFO] PID=%lu TID=%lu\n", ts,
            GetCurrentProcessId(), GetCurrentThreadId());
        fflush(g_file);

        InstallCrashHandler();
        return true;
    }

    void Shutdown() {
        if (!g_init.exchange(false, std::memory_order_relaxed)) return;
        std::lock_guard<std::mutex> lk(g_mtx);
        if (g_file) {
            char ts[24]; Stamp(ts, sizeof(ts));
            fprintf(g_file, "[%s][INFO] Log closing (uptime %.1fs)\n",
                ts, NowSec() - g_start.load(std::memory_order_relaxed));
            fclose(g_file);
            g_file = nullptr;
        }
        g_buckets.clear();
    }
}