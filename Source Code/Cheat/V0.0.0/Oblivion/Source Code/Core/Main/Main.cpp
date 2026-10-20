#include <Windows.h>
#include <mmsystem.h>
#include <imgui.h>

#include "../Overlay/Overlay.hpp"
#include "../Memory/Memory.hpp"
#include "../UI/UI.hpp"
#include "../../SDK/Entities/Entities.hpp"
#include "../../Utilities/Config/Config.hpp"
#include "../../Utilities/Logging/Logging.hpp"
#include "../../External/Xorstr/xorstr.hpp"
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

#pragma warning(push)
#pragma warning(disable: 28251)

bool g_unload_requested = false;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    timeBeginPeriod(1);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    Logging::Init();

    config.Load(xorstr_("config.ini"));
    Logging::Info("Config loaded (safe_mode=%d, esp_enemy=%d, esp_team=%d)",
        config.safe_mode ? 1 : 0,
        config.esp_enemy_enable ? 1 : 0,
        config.esp_team_enable ? 1 : 0);

    g_AllowMemoryWrite.store(!config.safe_mode, std::memory_order_relaxed);

    if (!mem.Init(xorstr_(L"cs2.exe"))) {
        Logging::Error("mem.Init failed (pid=%u)", mem.pid);
        timeEndPeriod(1);
        return 1;
    }
    Logging::Info("mem.Init OK (pid=%u, client=0x%llX, engine=0x%llX)",
        mem.pid,
        static_cast<unsigned long long>(mem.client),
        static_cast<unsigned long long>(mem.engine));

    Overlay::Config ocfg;
    ocfg.windowTitle = L"Overlay";
    ocfg.targetTitle = L"Counter-Strike 2";
    ocfg.hideWhenTargetInactive = false;
    ocfg.vsync = true;
    ocfg.targetFps = 0.0f;
    ocfg.interactKey = VK_INSERT;
    ocfg.exitKey = 0;
    ocfg.fontPath = "C:\\Windows\\Fonts\\arialbd.ttf";
    ocfg.fontSize = 18.0f;

    Overlay overlay;
    if (!overlay.Create(ocfg)) {
        Logging::Error("overlay.Create failed: %s", overlay.LastError().c_str());
        MessageBoxA(nullptr, overlay.LastError().c_str(), "Overlay Error", MB_OK | MB_ICONERROR);
        mem.Cleanup(); timeEndPeriod(1);
        return 1;
    }

    overlay.SetInteractive(true);
    Logging::Info("Overlay created");

    overlay.Run([&](Overlay& ov) {
        const double cbStart = FrameTime();

        const bool menuOpen = ov.IsInteractive();
        config.show_menu = menuOpen;

        const int w = static_cast<int>(ov.Size().x);
        const int h = static_cast<int>(ov.Size().y);

        const double uiStart = FrameTime();
        UI::Render(ov.Device(), ov.Context(), w, h);
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

        Logging::Sample("render_callback_ms", (FrameTime() - cbStart) * 1000.0);
        Logging::Tick();

        if (g_unload_requested) ov.RequestExit();
        });

    overlay.Shutdown();
    Logging::Info("Shutdown complete");

    mem.Cleanup();
    timeEndPeriod(1);
    Logging::Shutdown();
    return 0;
}

#pragma warning(pop)