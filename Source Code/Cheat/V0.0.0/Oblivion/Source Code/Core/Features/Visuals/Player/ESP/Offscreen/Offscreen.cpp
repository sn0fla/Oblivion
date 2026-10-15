#include "Offscreen.hpp"
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

        const bool enabled = isEnemy ? config.esp_enemy_offscreen
            : config.esp_team_offscreen;
        if (!enabled) return;

        const float* colArr = isEnemy ? config.esp_enemy_offscreen_color
            : config.esp_team_offscreen_color;
        const float distRatio = isEnemy ? config.esp_enemy_offscreen_distance
            : config.esp_team_offscreen_distance;
        const float arrowSize = isEnemy ? config.esp_enemy_offscreen_size
            : config.esp_team_offscreen_size;

        const ImU32 col = Col(colArr);
        const ImU32 outlineCol = IM_COL32(0, 0, 0, 180);

        const float cx = width * 0.5f;
        const float cy = height * 0.5f;

        const float minDim = static_cast<float>(width < height ? width : height);
        const float maxR = minDim * 0.5f - 40.0f;
        if (maxR <= 10.0f) return;

        const float r = maxR * distRatio;
        const float half = arrowSize * 0.5f;

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;

            Vector2 scr;
            const bool onScreen = Math::WorldToScreen(
                ent.origin, scr, viewMatrix, width, height);

            if (onScreen &&
                scr.x >= 0.0f && scr.x < static_cast<float>(width) &&
                scr.y >= 0.0f && scr.y < static_cast<float>(height))
            {
                continue;
            }

            float dx, dy;
            if (onScreen) {
                dx = scr.x - cx;
                dy = scr.y - cy;
            }
            else {
                const Vector3 delta = ent.origin - frame.localOrigin;
                const float rx = viewMatrix[0][0] * delta.x
                    + viewMatrix[0][1] * delta.y
                    + viewMatrix[0][2] * delta.z;
                const float uy = viewMatrix[1][0] * delta.x
                    + viewMatrix[1][1] * delta.y
                    + viewMatrix[1][2] * delta.z;
                dx = rx;
                dy = -uy;
            }

            const float len = sqrtf(dx * dx + dy * dy);
            if (len < 0.001f) continue;

            const float nx = dx / len;
            const float ny = dy / len;

            const float ax = cx + nx * r;
            const float ay = cy + ny * r;

            const float perpX = -ny;
            const float perpY = nx;

            const ImVec2 tip(ax + nx * half, ay + ny * half);
            const ImVec2 left(
                ax - nx * half + perpX * half,
                ay - ny * half + perpY * half);
            const ImVec2 right(
                ax - nx * half - perpX * half,
                ay - ny * half - perpY * half);

            dl->AddTriangleFilled(tip, left, right, col);
            dl->AddTriangle(tip, left, right, outlineCol, 1.0f);
        }
    }

    void RenderOffscreenESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable) RenderSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable)  RenderSide(frame, viewMatrix, width, height, false);
    }
}