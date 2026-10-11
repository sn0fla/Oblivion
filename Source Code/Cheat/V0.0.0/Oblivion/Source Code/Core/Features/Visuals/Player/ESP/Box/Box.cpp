#include "Box.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include "../../../../../../Utilities/Config/Config.hpp"
#include "../../../../../../External/ImGUI/imgui.h"
#include "../../../../../../SDK/Entities/Entities.hpp"
#include <algorithm>
#include <cmath>

namespace Features::ESP {

    static inline ImU32 ToImCol(const float c[4]) {
        return IM_COL32(
            static_cast<int>(c[0] * 255.0f + 0.5f),
            static_cast<int>(c[1] * 255.0f + 0.5f),
            static_cast<int>(c[2] * 255.0f + 0.5f),
            static_cast<int>(c[3] * 255.0f + 0.5f));
    }

    struct SideCfg {
        bool  enabled, boxEnabled, cornerStyle, rounded, fillStyle, outlineStyle;
        ImU32 boxCol, fillCol, outlineCol;
    };

    static inline void FillSideCfg(SideCfg& c,
        bool enabled, bool boxEnabled, bool cornerStyle, bool rounded,
        bool fillStyle, bool outlineStyle,
        const float boxCol[4], const float fillCol[4], const float outlineCol[4])
    {
        c.enabled = enabled; c.boxEnabled = boxEnabled;
        c.cornerStyle = cornerStyle; c.rounded = rounded;
        c.fillStyle = fillStyle; c.outlineStyle = outlineStyle;
        c.boxCol = ToImCol(boxCol); c.fillCol = ToImCol(fillCol); c.outlineCol = ToImCol(outlineCol);
    }

    static void DrawOutlinePass(ImDrawList* dl,
        float left, float topY, float right, float botY,
        float cornerLen, float rounding,
        bool cornerStyle, bool rounded, float thickness, ImU32 col)
    {
        if (cornerStyle) {
            if (rounded) {
                dl->AddBezierQuadratic(ImVec2(left, topY + cornerLen), ImVec2(left, topY),
                    ImVec2(left + cornerLen, topY), col, thickness);
                dl->AddBezierQuadratic(ImVec2(right - cornerLen, topY), ImVec2(right, topY),
                    ImVec2(right, topY + cornerLen), col, thickness);
                dl->AddBezierQuadratic(ImVec2(left, botY - cornerLen), ImVec2(left, botY),
                    ImVec2(left + cornerLen, botY), col, thickness);
                dl->AddBezierQuadratic(ImVec2(right - cornerLen, botY), ImVec2(right, botY),
                    ImVec2(right, botY - cornerLen), col, thickness);
            }
            else {
                dl->AddLine(ImVec2(left, topY), ImVec2(left + cornerLen, topY), col, thickness);
                dl->AddLine(ImVec2(left, topY), ImVec2(left, topY + cornerLen), col, thickness);
                dl->AddLine(ImVec2(right, topY), ImVec2(right - cornerLen, topY), col, thickness);
                dl->AddLine(ImVec2(right, topY), ImVec2(right, topY + cornerLen), col, thickness);
                dl->AddLine(ImVec2(left, botY), ImVec2(left + cornerLen, botY), col, thickness);
                dl->AddLine(ImVec2(left, botY), ImVec2(left, botY - cornerLen), col, thickness);
                dl->AddLine(ImVec2(right, botY), ImVec2(right - cornerLen, botY), col, thickness);
                dl->AddLine(ImVec2(right, botY), ImVec2(right, botY - cornerLen), col, thickness);
            }
        }
        else {
            const ImDrawFlags flags = rounded ? ImDrawFlags_RoundCornersAll : 0;
            dl->AddRect(ImVec2(left, topY), ImVec2(right, botY), col, rounding, flags, thickness);
        }
    }

    void RenderBoxESP(const FrameSnapshot& frame, const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        const float fw = static_cast<float>(width);
        const float fh = static_cast<float>(height);

        const float scale = fh / 1080.0f;
        const float boxThickness = (std::max)(1.0f, std::round(scale));
        const float outlinePad = (std::max)(1.0f, std::round(scale));
        const float outlineThickness = boxThickness + outlinePad * 2.0f;

        SideCfg sides[2];
        FillSideCfg(sides[0],
            config.esp_team_enable, config.esp_team_box,
            config.esp_team_box_style_corner, config.esp_team_box_style_rounded,
            config.esp_team_box_style_fill, config.esp_team_box_outline,
            config.esp_team_box_color, config.esp_team_box_fill_color, config.esp_team_box_outline_color);
        FillSideCfg(sides[1],
            config.esp_enemy_enable, config.esp_enemy_box,
            config.esp_enemy_box_style_corner, config.esp_enemy_box_style_rounded,
            config.esp_enemy_box_style_fill, config.esp_enemy_box_outline,
            config.esp_enemy_box_color, config.esp_enemy_box_fill_color, config.esp_enemy_box_outline_color);

        const ViewMatrix& vm = viewMatrix;
        const uint8_t localTeam = frame.localTeam;

        const float marginX = fw * 0.5f;
        const float marginY = fh * 0.5f;
        const float maxRawH = fh * 1.5f;

        for (const auto& ent : frame.entities) {
            const SideCfg& cfg = sides[(ent.team != localTeam) ? 1 : 0];
            if (!cfg.enabled || !cfg.boxEnabled) continue;
            if (!ent.isAlive || ent.isDormant) continue;

            Vector2 feetScreen, headScreen;
            if (!Math::WorldToScreen(ent.feetPos, feetScreen, vm, width, height)) continue;
            if (!Math::WorldToScreen(ent.headPos, headScreen, vm, width, height)) continue;

            const float fxs = feetScreen.x;
            const float fys = feetScreen.y;
            const float hxs = headScreen.x;
            const float hys = headScreen.y;

            if (!std::isfinite(fxs) || !std::isfinite(fys) ||
                !std::isfinite(hxs) || !std::isfinite(hys)) continue;

            const float rawH = fys - hys;
            if (!(rawH >= 1.0f) || rawH > maxRawH) continue;

            const float centerX = (fxs + hxs) * 0.5f;

            if (centerX < -marginX || centerX > fw + marginX) continue;
            if (fys < -marginY || fys > fh + marginY) continue;

            const float closeT = (std::clamp)(rawH / (fh * 0.75f), 0.0f, 1.0f);

            const float kTopMul = 0.05f + closeT * 0.18f;
            const float kBotMul = 0.05f + closeT * 0.06f;
            const float kAspect = 0.45f + closeT * 0.30f;

            const float boxH = rawH * (1.0f + kTopMul + kBotMul);
            const float boxW = boxH * kAspect;

            if (boxH > fh * 1.5f) continue;
            if (boxW > fw * 1.5f) continue;

            const float topY = hys - rawH * kTopMul;
            const float botY = topY + boxH;

            const float left = centerX - (boxW * 0.5f);
            const float right = left + boxW;

            const ImVec2 tl(left, topY);
            const ImVec2 br(right, botY);

            const float minDim = (std::min)(boxW, boxH);
            const float cornerLen = (std::min)(minDim * 0.22f, boxW * 0.35f);
            const float rounding = cfg.rounded ? (cfg.cornerStyle ? (std::min)(cornerLen, 12.0f) : (std::min)(minDim * 0.12f, 6.0f)) : 0.0f;

            if (cfg.fillStyle)
                dl->AddRectFilled(tl, br, cfg.fillCol, rounding, cfg.rounded ? ImDrawFlags_RoundCornersAll : 0);

            if (cfg.outlineStyle)
                DrawOutlinePass(dl, left, topY, right, botY, cornerLen, rounding, cfg.cornerStyle, cfg.rounded, outlineThickness, cfg.outlineCol);

            DrawOutlinePass(dl, left, topY, right, botY, cornerLen, rounding, cfg.cornerStyle, cfg.rounded, boxThickness, cfg.boxCol);
        }
    }
}