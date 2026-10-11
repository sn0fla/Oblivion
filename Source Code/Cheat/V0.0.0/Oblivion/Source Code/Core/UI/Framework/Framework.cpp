#include "Framework.hpp"

namespace Framework {

    static ImFont* g_IconFont = nullptr;

    void ResetCachedFonts() {
        g_IconFont = nullptr;
    }

    ImFont* IconFont() {
        if (g_IconFont) return g_IconFont;

        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig cfg;
        cfg.OversampleH = 3;
        cfg.OversampleV = 3;
        cfg.PixelSnapH = true;

        static const ImWchar ranges[] = {
            0xE700, 0xE7FF,
            0xE800, 0xE8FF,
            0xE900, 0xE9FF,
            0xEA00, 0xEAFF,
            0x0000
        };

        const char* candidates[] = {
            "C:\\Windows\\Fonts\\segmdl2.ttf",
            "C:\\Windows\\Fonts\\SegoeIcons.ttf",
            "C:\\Windows\\Fonts\\seguisym.ttf",
        };

        for (const char* path : candidates) {
            g_IconFont = io.Fonts->AddFontFromFileTTF(path, 18.0f, &cfg, ranges);
            if (g_IconFont) break;
        }

        if (!g_IconFont) g_IconFont = ImGui::GetFont();
        return g_IconFont;
    }
}