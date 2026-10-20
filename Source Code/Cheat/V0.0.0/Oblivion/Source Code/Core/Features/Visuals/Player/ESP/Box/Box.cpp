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

        const uint8_t localTeam = frame.localTeam;

        for (const auto& ent : frame.entities) {
            const SideCfg& cfg = sides[(ent.team != localTeam) ? 1 : 0];
            if (!cfg.enabled || !cfg.boxEnabled) continue;
            if (!ent.isAlive || ent.isDormant) continue;

            BoxRect box = ComputeBox(ent, viewMatrix, width, height);
            if (!box.valid) continue;

            const ImVec2 tl(box.left, box.top);
            const ImVec2 br(box.right, box.bottom);

            const float minDim = (std::min)(box.width(), box.height());
            const float cornerLen = (std::min)(minDim * 0.22f, box.width() * 0.35f);
            const float rounding = cfg.rounded ? (cfg.cornerStyle ? (std::min)(cornerLen, 12.0f) : (std::min)(minDim * 0.12f, 6.0f)) : 0.0f;

            if (cfg.fillStyle)
                dl->AddRectFilled(tl, br, cfg.fillCol, rounding, cfg.rounded ? ImDrawFlags_RoundCornersAll : 0);

            if (cfg.outlineStyle)
                DrawOutlinePass(dl, box.left, box.top, box.right, box.bottom, cornerLen, rounding, cfg.cornerStyle, cfg.rounded, outlineThickness, cfg.outlineCol);

            DrawOutlinePass(dl, box.left, box.top, box.right, box.bottom, cornerLen, rounding, cfg.cornerStyle, cfg.rounded, boxThickness, cfg.boxCol);
        }
    }
}