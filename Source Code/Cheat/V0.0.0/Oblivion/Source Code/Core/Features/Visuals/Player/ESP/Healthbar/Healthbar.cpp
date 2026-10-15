#include "Healthbar.hpp"
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

        const float fh = static_cast<float>(height);

        const float* colArr = isEnemy ? config.esp_enemy_healthbar_color
            : config.esp_team_healthbar_color;
        const ImU32 fgCol = Col(colArr);
        const ImU32 bgCol = IM_COL32(0, 0, 0, 190);

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;
            if (ent.health == 0 || ent.health > 100) continue;

            Vector2 feetScreen, topScreen, centerScreen;
            if (!Math::WorldToScreen(ent.feetPos, feetScreen, viewMatrix, width, height)) continue;
            if (!Math::WorldToScreen(ent.headPos, topScreen, viewMatrix, width, height)) continue;

            const Vector3 centerWorld = ent.hasBones
                ? ent.bones[static_cast<int>(BoneIndex::Spine_1)]
                : ent.origin;

            const bool centerOk = Math::WorldToScreen(centerWorld, centerScreen, viewMatrix, width, height);

            if (!std::isfinite(feetScreen.y) || !std::isfinite(topScreen.y)) continue;

            const float centerX = centerOk ? centerScreen.x : feetScreen.x;
            if (!std::isfinite(centerX)) continue;

            const float screenH = feetScreen.y - topScreen.y;
            if (screenH < 1.0f) continue;

            const float closeT = (std::clamp)(screenH / (fh * 0.75f), 0.0f, 1.0f);

            const float kTopMul = 0.05f + closeT * 0.18f;
            const float kBotMul = 0.05f + closeT * 0.06f;
            const float kAspect = 0.45f + closeT * 0.30f;

            const float ratio = ent.health / 100.0f;

            const float h = screenH * (1.0f + kTopMul + kBotMul);
            const float w = h * kAspect;

            if (h > fh * 1.5f) continue;
            if (w > static_cast<float>(width) * 1.5f) continue;

            const float top = topScreen.y - screenH * kTopMul;
            const float left = centerX - w * 0.5f;

            const float barW = (std::clamp)(screenH * 0.028f, 2.0f, 7.0f);
            const float barGap = barW * 2.2f;
            const float barRound = barW * 0.35f;

            const ImVec2 barMin(left - barGap - barW, top);
            const ImVec2 barMax(left - barGap, top + h);

            dl->AddRectFilled(barMin, barMax, bgCol, barRound);

            const float fillH = h * ratio;
            const ImVec2 fillMin(barMin.x, barMax.y - fillH);

            if (fillH > barW * 0.5f)
                dl->AddRectFilled(fillMin, barMax, fgCol, barRound);
            else
                dl->AddRectFilled(fillMin, barMax, fgCol, 0.0f);

            dl->AddRect(barMin, barMax, IM_COL32(255, 255, 255, 60), barRound, 0, 1.0f);
        }
    }

    void RenderHealthbarESP(const FrameSnapshot& frame, const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_healthbar)
            RenderSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_healthbar)
            RenderSide(frame, viewMatrix, width, height, false);
    }
}