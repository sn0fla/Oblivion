#include <Windows.h>
#include <mmsystem.h>
#include <imgui.h>
#include <algorithm>

#include "../Overlay/Overlay.hpp"
#include "../Memory/Memory.hpp"
#include "../UI/UI.hpp"
#include "../../SDK/Entities/Entities.hpp"
#include "../../SDK/Schemas/offsets.hpp"
#include "../../SDK/Schemas/client_dll.hpp"
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
#include "../../Utilities/Config/Config.hpp"
#include "../../External/Xorstr/xorstr.hpp"

#pragma comment(lib, "winmm.lib")

#pragma warning(push)
#pragma warning(disable: 28251)

bool g_unload_requested = false;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    timeBeginPeriod(1);
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    config.Load(xorstr_("config.ini"));

    g_AllowMemoryWrite.store(!config.safe_mode, std::memory_order_relaxed);

    if (!mem.Init(xorstr_(L"cs2.exe"))) {
        timeEndPeriod(1);
        return 1;
    }

    Overlay::Config ocfg;
    ocfg.windowTitle = L"Overlay";
    ocfg.targetTitle = L"Counter-Strike 2";
    ocfg.hideWhenTargetInactive = false;
    ocfg.vsync = false;
    ocfg.targetFps = 144.0f;
    ocfg.interactKey = VK_INSERT;
    ocfg.exitKey = 0;
    ocfg.fontPath = "C:\\Windows\\Fonts\\arialbd.ttf";
    ocfg.fontSize = 18.0f;

    Overlay overlay;
    if (!overlay.Create(ocfg)) {
        MessageBoxA(nullptr, overlay.LastError().c_str(), "Overlay Error", MB_OK | MB_ICONERROR);
        mem.Cleanup(); timeEndPeriod(1);
        return 1;
    }

    overlay.SetInteractive(true);

    StartEntityThread();

    overlay.Run([&](Overlay& ov) {
        const double cbStart = FrameTime();

        const bool menuOpen = ov.IsInteractive();
        config.show_menu = menuOpen;

        const int w = static_cast<int>(ov.Size().x);
        const int h = static_cast<int>(ov.Size().y);

        const double uiStart = FrameTime();
        UI::Render(ov.Device(), ov.Context(), w, h);
        g_uiRenderMs.store((FrameTime() - uiStart) * 1000.0, std::memory_order_relaxed);

        static FrameSnapshot frame;
        if (frame.entities.capacity() < 64) {
            frame.entities.reserve(64);
        }

        const double snapStart = FrameTime();
        AcquireFrameSnapshot(frame);
        g_snapshotWaitMs.store((FrameTime() - snapStart) * 1000.0, std::memory_order_relaxed);

        ViewMatrix vm{};
        if (mem.client) {
            mem.ReadRaw(mem.client + cs2_dumper::offsets::client_dll::dwViewMatrix,
                &vm, sizeof(ViewMatrix));
        }

        if (frame.publishTime > 0.0 && mem.client) {
            g_renderEntityCount.store(static_cast<int>(frame.entities.size()),
                std::memory_order_relaxed);
            g_snapshotAgeAtRenderMs.store(
                (FrameTime() - frame.publishTime) * 1000.0,
                std::memory_order_relaxed);

            const double espStart = FrameTime();
            Features::ESP::RenderSnaplineESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderBoxESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderSkeletonESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderViewDirectionESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderHeadCircleESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderHealthbarESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderNametagESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderDistanceESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderWeaponESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderFlagsESP(frame, vm, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderOffscreenESP(frame, vm, ov.Device(), ov.Context(), w, h);
            g_espRenderMs.store((FrameTime() - espStart) * 1000.0, std::memory_order_relaxed);
        }

        g_renderCallbackMs.store((FrameTime() - cbStart) * 1000.0, std::memory_order_relaxed);

        if (g_unload_requested) ov.RequestExit();
        });

    StopEntityThread();
    overlay.Shutdown();

    mem.Cleanup();
    timeEndPeriod(1);
    return 0;
}

#pragma warning(pop)