#pragma once
#include <imgui.h>

namespace UI {
    namespace Internal {

        extern int   g_activeTab;
        extern float g_iconHover[5];
        extern float g_iconSel[5];

        extern int   g_activeSub;
        extern float g_subHover[4];
        extern float g_subSel[4];

        extern float g_espHover[2][24];
        extern float g_espSel[2][24];
        extern float g_espColorHover[2][24];

        extern bool  g_espExpanded[2];
        extern float g_espExpandT[2];
        extern float g_espExpanderHover[2];

        extern bool  g_flagsExpanded[2];
        extern float g_flagsExpandT[2];
        extern float g_flagsExpanderHover[2];

        ImU32 ScaleAlpha(ImU32 col, float mul);
        float Hash01(unsigned int x);
        void PlayClickSound();
        ImFont* GetIconFont();
        void DrawFlowLine(ImDrawList* dl, ImVec2 a, ImVec2 b,
            ImU32 baseCol, ImU32 glowCol, ImU32 coreCol,
            float thickness, float pulsePos, float pulseWidth);
        void DrawIconGlyph(ImDrawList* dl, int type, ImVec2 c,
            float boxSz, float fontSz, ImU32 col);

    }
}