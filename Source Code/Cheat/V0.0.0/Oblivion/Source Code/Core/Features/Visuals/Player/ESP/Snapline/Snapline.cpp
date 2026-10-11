#include "Snapline.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include "../../../../../../Utilities/Config/Config.hpp"
#include "../../../../../../External/ImGUI/imgui.h"
#include <cmath>

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

        const float* colArr = isEnemy ? config.esp_enemy_snapline_color
            : config.esp_team_snapline_color;
        const ImU32 col = Col(colArr);

        const ImVec2 origin(width * 0.5f, static_cast<float>(height));

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;

            Vector2 feetScreen;
            if (!Math::WorldToScreen(ent.feetPos, feetScreen, viewMatrix, width, height))
                continue;

            if (!std::isfinite(feetScreen.x) || !std::isfinite(feetScreen.y))
                continue;

            dl->AddLine(origin, ImVec2(feetScreen.x, feetScreen.y), col, 1.0f);
        }
    }

    void RenderSnaplineESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_snapline)
            RenderSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_snapline)
            RenderSide(frame, viewMatrix, width, height, false);
    }
}