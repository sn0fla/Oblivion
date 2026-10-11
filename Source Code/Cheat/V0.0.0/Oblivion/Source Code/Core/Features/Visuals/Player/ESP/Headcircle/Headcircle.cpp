#include "HeadCircle.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include "../../../../../../Utilities/Config/Config.hpp"
#include "../../../../../../External/ImGUI/imgui.h"
#include <cmath>
#include <algorithm>

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

        const float* colArr = isEnemy ? config.esp_enemy_headcircle_color
            : config.esp_team_headcircle_color;
        const ImU32 col = Col(colArr);

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;

            const Vector3 headPos = ent.hasBones
                ? ent.bones[static_cast<int>(BoneIndex::Head)]
                : ent.headPos;

            Vector2 headScreen;
            if (!Math::WorldToScreen(headPos, headScreen, viewMatrix, width, height))
                continue;

            const Vector3 offsetPos(headPos.x, headPos.y, headPos.z + 8.0f);
            Vector2 offsetScreen;
            float radius = 6.0f;

            if (Math::WorldToScreen(offsetPos, offsetScreen, viewMatrix, width, height)) {
                const float dx = offsetScreen.x - headScreen.x;
                const float dy = offsetScreen.y - headScreen.y;
                radius = sqrtf(dx * dx + dy * dy);
                radius = (std::clamp)(radius, 2.0f, 30.0f);
            }

            int segments = 12 + static_cast<int>(radius * 0.5f);
            segments = (std::clamp)(segments, 12, 64);

            dl->AddCircle(ImVec2(headScreen.x, headScreen.y),
                radius, col, segments, 2.0f);
        }
    }

    void RenderHeadCircleESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_headcircle)
            RenderSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_headcircle)
            RenderSide(frame, viewMatrix, width, height, false);
    }
}