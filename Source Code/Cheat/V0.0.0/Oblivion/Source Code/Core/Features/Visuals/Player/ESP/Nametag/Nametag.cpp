#include "Nametag.hpp"
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
        ImFont* font = ImGui::GetFont();
        if (!font) return;

        const float* colArr = isEnemy ? config.esp_enemy_name_color
            : config.esp_team_name_color;
        const ImU32 textCol = Col(colArr);

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;
            if (ent.name[0] == '\0') continue;

            BoxRect box = ComputeBox(ent, viewMatrix, width, height);
            if (!box.valid) continue;

            const float fontSize = (std::clamp)(box.height() * 0.11f, 10.0f, 18.0f);
            const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, ent.name);
            if (textSize.x <= 0.0f) continue;

            const float gap = fontSize * 0.25f;
            const float textX = box.centerX - textSize.x * 0.5f;
            const float textY = box.top - gap - textSize.y;

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