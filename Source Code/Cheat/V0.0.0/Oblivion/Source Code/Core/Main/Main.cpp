#include <Windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <imgui.h>
#include <atomic>
#include <thread>

#include "../Overlay/Overlay.hpp"
#include "../Memory/Memory.hpp"
#include "../GUI/GUI.hpp"
#include "../GUI/Animations/Animations.hpp"
#include "../../SDK/Entities/Entities.hpp"
#include "../../Utilities/Config/Config.hpp"
#include "../../Utilities/Logging/Logging.hpp"
#include "../../External/Xorstr/xorstr.hpp"
#include "../Features/Combat/Aimbot/Aimbot.hpp"
#include "../Features/Visuals/Player/ESP/Box/Box.hpp"
#include "../Features/Visuals/Player/ESP/Skeleton/Skeleton.hpp"
#include "../Features/Visuals/Player/ESP/Healthbar/Healthbar.hpp"
#include "../Features/Visuals/Player/ESP/Nametag/Nametag.hpp"
#include "../Features/Visuals/Player/ESP/Distance/Distance.hpp"
#include "../Features/Visuals/Player/ESP/HeadCircle/HeadCircle.hpp"
#include "../Features/Visuals/Player/ESP/Snapline/Snapline.hpp"
#include "../Features/Visuals/Player/ESP/ViewDirection/ViewDirection.hpp"
#include "../Features/Visuals/Player/ESP/Weapon/Weapon.hpp"
#include "../Features/Visuals/Player/ESP/Flags/Flags.hpp"
#include "../Features/Visuals/Player/ESP/Offscreen/Offscreen.hpp"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

#pragma warning(push)
#pragma warning(disable: 28251)

std::atomic<bool> g_unload_requested{ false };

static bool HasUiActiveFlag()
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return false;
    bool found = false;
    for (int i = 1; i < argc; ++i)
    {
        if (wcscmp(argv[i], L"--ui-active") == 0) { found = true; break; }
    }
    LocalFree(argv);
    return found;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Logging::Init();

    if (config.fullscreen_mode && !HasUiActiveFlag() && !Overlay::HasUIAccess()) {
        if (Overlay::TryElevateToUIAccess()) {
            Logging::Shutdown();
            return 0;
        }
    }

    timeBeginPeriod(1);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    Logging::Info("Config loaded (safe_mode=%d, esp_enemy=%d, esp_team=%d, fullscreen_mode=%d)",
        config.safe_mode ? 1 : 0,
        config.esp_enemy_enable ? 1 : 0,
        config.esp_team_enable ? 1 : 0,
        config.fullscreen_mode ? 1 : 0);

    g_AllowMemoryWrite.store(!config.safe_mode, std::memory_order_relaxed);

    Overlay::Config ocfg;
    ocfg.windowTitle = Rng::WStr(10, true);
    ocfg.targetTitle = L"Counter-Strike 2";
    ocfg.hideWhenTargetInactive = false;
    ocfg.vsync = config.vsync;
    ocfg.allow_tearing = config.allow_tearing;
    ocfg.targetFps = 0.0f;
    ocfg.interactKey = VK_INSERT;
    ocfg.exitKey = 0;
    ocfg.fontPath = "C:\\Windows\\Fonts\\arialbd.ttf";
    ocfg.fontSize = 18.0f;

    Overlay overlay;
    if (!overlay.Create(ocfg)) {
        Logging::Error("overlay.Create failed: %s", overlay.LastError().c_str());
        MessageBoxA(nullptr, overlay.LastError().c_str(), "Overlay Error", MB_OK | MB_ICONERROR);
        timeEndPeriod(1);
        return 1;
    }

    overlay.SetInteractive(true);
    Animations::BootStart();
    Logging::Info("Overlay created (boot animation started)");

    std::thread memThread([&] {
        if (g_StopMemInit.load(std::memory_order_relaxed)) return;

        if (mem.Init(xorstr_(L"cs2.exe"))) {
            Logging::Info("mem.Init OK (pid=%u, client=0x%llX, engine=0x%llX)",
                mem.pid,
                static_cast<unsigned long long>(mem.client),
                static_cast<unsigned long long>(mem.engine));

            while (!g_StopMemInit.load(std::memory_order_relaxed)) {
                if (!mem.handle) break;

                DWORD wait = WaitForSingleObject(mem.handle, 500);

                if (wait == WAIT_OBJECT_0) {
                    Logging::Info("cs2.exe exited (WAIT_OBJECT_0), requesting shutdown");
                    g_unload_requested.store(true, std::memory_order_release);
                    return;
                }

                if (wait == WAIT_FAILED) {
                    // Handle likely lacks SYNCHRONIZE. Fall back to exit-code polling.
                    DWORD exitCode = 0;
                    if (GetExitCodeProcess(mem.handle, &exitCode)) {
                        if (exitCode != STILL_ACTIVE) {
                            Logging::Info("cs2.exe exited (exit code %u), requesting shutdown",
                                static_cast<unsigned>(exitCode));
                            g_unload_requested.store(true, std::memory_order_release);
                            return;
                        }
                        // Process still alive; wait a bit before retrying.
                        for (int i = 0; i < 50 && !g_StopMemInit.load(std::memory_order_relaxed); ++i) {
                            Sleep(10);
                        }
                    }
                    else {
                        // No query rights either — can't monitor. Stop watching but keep running.
                        Logging::Info("mem monitor: WaitForSingleObject/GetExitCodeProcess "
                            "unavailable (err=%lu); stopping monitor (cheat stays loaded)",
                            GetLastError());
                        return;
                    }
                }
                // WAIT_TIMEOUT: normal, loop again.
            }
        }
        else {
            if (g_StopMemInit.load(std::memory_order_relaxed)) return;
            Logging::Error("mem.Init failed (pid=%u)", mem.pid);
        }
        });

    overlay.Run([&](Overlay& ov) {
        ov.SetVsync(config.vsync);
        ov.SetAllowTearing(config.allow_tearing);

        Animations::BootTick();

        const double cbStart = FrameTime();

        const bool menuOpen = ov.IsInteractive();
        config.show_menu = menuOpen && !Animations::IsBooting();

        const int w = static_cast<int>(ov.Size().x);
        const int h = static_cast<int>(ov.Size().y);

        const double uiStart = FrameTime();
        GUI::Render(ov.Device(), ov.Context(), w, h);
        Logging::Sample("ui_render_ms", (FrameTime() - uiStart) * 1000.0);

        static FrameSnapshot frame;
        if (frame.entities.capacity() < 64)
            frame.entities.reserve(64);

        const double snapStart = FrameTime();
        AcquireRefreshedFrameSnapshot(frame);
        Logging::Sample("snapshot_wait_ms", (FrameTime() - snapStart) * 1000.0);

        if (frame.publishTime > 0.0 && mem.client) {
            const double age = FrameTime() - frame.publishTime;
            Logging::Sample("snapshot_age_at_render_ms", age * 1000.0);
            Logging::Sample("render_entity_count", static_cast<double>(frame.entities.size()));

            ov.SetLastPublishSec(frame.publishTime);

            const double aimStart = FrameTime();
            Aimbot::Tick(frame);
            Logging::Sample("aimbot_tick_ms", (FrameTime() - aimStart) * 1000.0);

            const double espStart = FrameTime();
            Features::ESP::RenderSnaplineESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderBoxESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderSkeletonESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderViewDirectionESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderHeadCircleESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderHealthbarESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderNametagESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderDistanceESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderWeaponESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderFlagsESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderOffscreenESP(frame, frame.viewMatrix, ov.Device(), ov.Context(), w, h);
            Logging::Sample("esp_render_ms", (FrameTime() - espStart) * 1000.0);
        }

        Animations::Render();

        Logging::Sample("render_callback_ms", (FrameTime() - cbStart) * 1000.0);
        Logging::Tick();

        if (g_unload_requested.load(std::memory_order_acquire)) ov.RequestExit();
        });

    g_StopMemInit.store(true, std::memory_order_relaxed);
    if (memThread.joinable()) memThread.join();

    overlay.Shutdown();
    Logging::Info("Shutdown complete");

    mem.Cleanup();
    timeEndPeriod(1);
    Logging::Shutdown();
    return 0;
}

#pragma warning(pop)