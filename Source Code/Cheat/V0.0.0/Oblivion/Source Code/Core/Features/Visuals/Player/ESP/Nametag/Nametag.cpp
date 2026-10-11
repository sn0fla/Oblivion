#include "Nametag.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include "../../../../../../Utilities/Config/Config.hpp"
#include "../../../../../../External/ImGUI/imgui.h"
#include <algorithm>
#include <cmath>

namespace Features::ESP {

    static inline ImU32 Col(const float c[4]) {
        return IM_COL32(static_cast<int>(c[0] * 255.0f),
            static_cast<int>(c[1] * 255.0f),
            static_cast<int>(c[2] * 255.0f),
            static_cast<int>(c[3] * 255.0f));
    }

    static void RenderSide(const FrameSnapshot& frame, const ViewMatrix& viewMatrix,
        int width, int height, bool isEnemy)
    {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImFont* font = ImGui::GetFont();
        if (!font) return;

        const float fh = static_cast<float>(height);

        const float* colArr = isEnemy ? config.esp_enemy_name_color
            : config.esp_team_name_color;
        const ImU32 textCol = Col(colArr);

        constexpr float kGapRatio = 0.18f;

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;
            if (ent.name[0] == '\0') continue;

            Vector2 feetScreen, topScreen;
            if (!Math::WorldToScreen(ent.feetPos, feetScreen, viewMatrix, width, height)) continue;
            if (!Math::WorldToScreen(ent.headPos, topScreen, viewMatrix, width, height)) continue;

            if (!std::isfinite(feetScreen.x) || !std::isfinite(feetScreen.y) ||
                !std::isfinite(topScreen.x) || !std::isfinite(topScreen.y)) continue;

            const float screenH = feetScreen.y - topScreen.y;
            if (screenH < 1.0f) continue;

            const float closeT = (std::clamp)(screenH / (fh * 0.75f), 0.0f, 1.0f);
            const float kTopMul = 0.05f + closeT * 0.18f;

            const float fontSize = (std::clamp)(screenH * 0.11f, 10.0f, 18.0f);
            const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, ent.name);
            if (textSize.x <= 0.0f) continue;

            const float gap = fontSize * kGapRatio;
            const float boxTopY = topScreen.y - screenH * kTopMul;

            const float boxCenterX = (feetScreen.x + topScreen.x) * 0.5f;
            const float textX = boxCenterX - textSize.x * 0.5f;
            const float textY = boxTopY - gap - textSize.y;

            constexpr float kShadowOff = 1.0f;
            const ImU32 shadow = IM_COL32(0, 0, 0, 200);

            dl->AddText(font, fontSize, ImVec2(textX - kShadowOff, textY), shadow, ent.name);
            dl->AddText(font, fontSize, ImVec2(textX + kShadowOff, textY), shadow, ent.name);
            dl->AddText(font, fontSize, ImVec2(textX, textY - kShadowOff), shadow, ent.name);
            dl->AddText(font, fontSize, ImVec2(textX, textY + kShadowOff), shadow, ent.name);
            dl->AddText(font, fontSize, ImVec2(textX, textY), textCol, ent.name);
        }
    }

    void RenderNametagESP(const FrameSnapshot& frame, const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_name)
            RenderSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_name)
            RenderSide(frame, viewMatrix, width, height, false);
    }
}