#include <Windows.h>
#include <mmsystem.h>
#include <imgui.h>

#include "../Overlay/Overlay.hpp"
#include "../Memory/Memory.hpp"
#include "../UI/UI.hpp"
#include "../../SDK/Entities/Entities.hpp"
#include "../../SDK/Schemas/offsets.hpp"
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
        const bool menuOpen = ov.IsInteractive();
        config.show_menu = menuOpen;

        const int w = static_cast<int>(ov.Size().x);
        const int h = static_cast<int>(ov.Size().y);

        UI::Render(ov.Device(), ov.Context(), w, h);

        const FrameSnapshot* frame = AcquireFrameSnapshot();
        if (frame && mem.client) {
            Features::ESP::RenderSnaplineESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderBoxESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderSkeletonESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderViewDirectionESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderHeadCircleESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderHealthbarESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderNametagESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderDistanceESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderWeaponESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);
            Features::ESP::RenderFlagsESP(*frame, frame->viewMatrix, ov.Device(), ov.Context(), w, h);

            ReleaseFrameSnapshot();
        }

        if (g_unload_requested) ov.RequestExit();
        });

    StopEntityThread();
    overlay.Shutdown();

    mem.Cleanup();
    timeEndPeriod(1);
    return 0;
}

#pragma warning(pop)
