#include "Healthbar.hpp"
#include "../Box/Box.hpp"
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

        const float* colArr = isEnemy ? config.esp_enemy_healthbar_color
            : config.esp_team_healthbar_color;
        const ImU32 fgCol = Col(colArr);
        const ImU32 bgCol = IM_COL32(0, 0, 0, 190);

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;
            if (ent.health == 0 || ent.health > 100) continue;

            BoxRect box = ComputeBox(ent, viewMatrix, width, height);
            if (!box.valid) continue;

            const float ratio = ent.health / 100.0f;
            const float barW = (std::clamp)(box.height() * 0.028f, 2.0f, 7.0f);
            const float barGap = barW * 1.5f;
            const float barRound = barW * 0.35f;

            const ImVec2 barMin(box.left - barGap - barW, box.top);
            const ImVec2 barMax(box.left - barGap, box.bottom);

            dl->AddRectFilled(barMin, barMax, bgCol, barRound);

            const float fillH = box.height() * ratio;
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