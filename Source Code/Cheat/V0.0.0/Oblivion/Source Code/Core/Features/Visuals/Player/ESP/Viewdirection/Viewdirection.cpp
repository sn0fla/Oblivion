#include "ViewDirection.hpp"
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

    static Vector3 AngleToDirection(const Vector3& ang) {
        constexpr float kDeg2Rad = 3.14159265358979323846f / 180.0f;
        const float pitch = ang.x * kDeg2Rad;
        const float yaw = ang.y * kDeg2Rad;
        Vector3 dir;
        dir.x = cosf(pitch) * cosf(yaw);
        dir.y = cosf(pitch) * sinf(yaw);
        dir.z = -sinf(pitch);
        return dir;
    }

    static void RenderSide(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix, int width, int height, bool isEnemy)
    {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();

        const float* colArr = isEnemy ? config.esp_enemy_viewdirection_color
            : config.esp_team_viewdirection_color;
        const ImVec4 baseColor(colArr[0], colArr[1], colArr[2], colArr[3]);

        constexpr float kFovHalf = 45.0f;
        constexpr float kLength = 120.0f;
        constexpr int   kLines = 21;

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;
            if (!ent.hasViewAngles) continue;

            const Vector3 viewAngles = ent.viewAngles;

            Vector2 screenStart;
            if (!Math::WorldToScreen(ent.headPos, screenStart, viewMatrix, width, height))
                continue;

            const float minAlpha = baseColor.w * 0.25f;
            const float maxAlpha = baseColor.w;

            for (int i = 0; i < kLines; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(kLines - 1);
                const float yawOffset = -kFovHalf + t * 2.0f * kFovHalf;
                const float centerDist = fabsf(t - 0.5f) * 2.0f;
                const float alpha = maxAlpha - (maxAlpha - minAlpha) * centerDist;

                Vector3 adjusted = viewAngles;
                adjusted.y += yawOffset;

                Vector3 dir = AngleToDirection(adjusted);
                Vector3 end = ent.headPos + dir * kLength;

                Vector2 screenEnd;
                if (!Math::WorldToScreen(end, screenEnd, viewMatrix, width, height))
                    continue;

                const ImU32 lineCol = ImGui::ColorConvertFloat4ToU32(
                    ImVec4(baseColor.x, baseColor.y, baseColor.z, alpha));
                const float thickness = 1.2f + 0.8f * (1.0f - centerDist);

                dl->AddLine(ImVec2(screenStart.x, screenStart.y),
                    ImVec2(screenEnd.x, screenEnd.y), lineCol, thickness);
            }

            Vector3 centerEnd = ent.headPos + AngleToDirection(viewAngles) * kLength;
            Vector2 screenCenterEnd;
            if (Math::WorldToScreen(centerEnd, screenCenterEnd, viewMatrix, width, height)) {
                const ImU32 centerCol = ImGui::ColorConvertFloat4ToU32(baseColor);
                dl->AddLine(ImVec2(screenStart.x, screenStart.y),
                    ImVec2(screenCenterEnd.x, screenCenterEnd.y), centerCol, 2.5f);
                dl->AddCircleFilled(
                    ImVec2(screenCenterEnd.x, screenCenterEnd.y), 3.0f, centerCol);
            }
        }
    }

    void RenderViewDirectionESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_viewdirection)
            RenderSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_viewdirection)
            RenderSide(frame, viewMatrix, width, height, false);
    }
}