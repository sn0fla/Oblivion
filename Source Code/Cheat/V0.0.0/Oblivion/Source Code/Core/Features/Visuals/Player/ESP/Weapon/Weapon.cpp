#include "Weapon.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include "../../../../../../Utilities/Config/Config.hpp"
#include "../../../../../../External/ImGUI/imgui.h"
#include <cmath>

namespace Features::ESP {

    static inline ImU32 WeaponCol(const float c[4]) {
        return IM_COL32(
            static_cast<int>(c[0] * 255.0f),
            static_cast<int>(c[1] * 255.0f),
            static_cast<int>(c[2] * 255.0f),
            static_cast<int>(c[3] * 255.0f));
    }

    static void RenderWeaponSide(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix, int width, int height, bool isEnemy)
    {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImFont* font = ImGui::GetFont();
        if (!font) return;

        const float* colArr = isEnemy ? config.esp_enemy_weapon_color
            : config.esp_team_weapon_color;
        const ImU32 textCol = WeaponCol(colArr);
        const ImU32 shadowCol = IM_COL32(0, 0, 0, 200);

        constexpr float kBottomOff = 22.0f;

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;
            if (ent.weaponName[0] == '\0') continue;

            Vector2 feetScreen, topScreen;
            if (!Math::WorldToScreen(ent.feetPos, feetScreen, viewMatrix, width, height))
                continue;
            if (!Math::WorldToScreen(ent.headPos, topScreen, viewMatrix, width, height))
                continue;

            if (!std::isfinite(feetScreen.x) || !std::isfinite(feetScreen.y) ||
                !std::isfinite(topScreen.x) || !std::isfinite(topScreen.y))
                continue;

            const float screenH = feetScreen.y - topScreen.y;
            if (screenH < 1.0f) continue;

            const float fontSize = (std::clamp)(screenH * 0.11f, 10.0f, 18.0f);

            const ImVec2 sz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, ent.weaponName);
            const float x = feetScreen.x - sz.x * 0.5f;
            const float y = feetScreen.y + kBottomOff;

            dl->AddText(font, fontSize,
                ImVec2(x + 1.0f, y + 1.0f), shadowCol, ent.weaponName);
            dl->AddText(font, fontSize,
                ImVec2(x, y), textCol, ent.weaponName);
        }
    }

    void RenderWeaponESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_weapon)
            RenderWeaponSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_weapon)
            RenderWeaponSide(frame, viewMatrix, width, height, false);
    }

}