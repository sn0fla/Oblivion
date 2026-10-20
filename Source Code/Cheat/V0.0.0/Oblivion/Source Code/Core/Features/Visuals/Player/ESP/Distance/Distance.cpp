#include "Distance.hpp"
#include "../Box/Box.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include "../../../../../../Utilities/Config/Config.hpp"
#include "../../../../../../External/ImGUI/imgui.h"
#include <cmath>
#include <cstdio>

namespace Features::ESP {

    static inline ImU32 Col(const float c[4]) {
        return IM_COL32(
            static_cast<int>(c[0] * 255.0f),
            static_cast<int>(c[1] * 255.0f),
            static_cast<int>(c[2] * 255.0f),
            static_cast<int>(c[3] * 255.0f));
    }

    static void RenderSide(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix, int width, int height, bool isEnemy)
    {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImFont* font = ImGui::GetFont();
        if (!font) return;

        const float* colArr = isEnemy ? config.esp_enemy_distance_color
            : config.esp_team_distance_color;
        const ImU32 textCol = Col(colArr);
        const ImU32 shadowCol = IM_COL32(0, 0, 0, 200);

        constexpr float kBottomOff = 6.0f;

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;

            BoxRect box = ComputeBox(ent, viewMatrix, width, height);
            if (!box.valid) continue;

            const float fontSize = (std::clamp)(box.height() * 0.11f, 10.0f, 18.0f);
            const float dist = ent.origin.DistTo(frame.localOrigin);

            char buf[32];
            snprintf(buf, sizeof(buf), "%.0fm", dist / 100.0f);

            const ImVec2 sz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, buf);
            const float x = box.centerX - sz.x * 0.5f;
            const float y = box.bottom + kBottomOff;

            dl->AddText(font, fontSize, ImVec2(x + 1.0f, y + 1.0f), shadowCol, buf);
            dl->AddText(font, fontSize, ImVec2(x, y), textCol, buf);
        }
    }

    void RenderDistanceESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_distance)
            RenderSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_distance)
            RenderSide(frame, viewMatrix, width, height, false);
    }
}