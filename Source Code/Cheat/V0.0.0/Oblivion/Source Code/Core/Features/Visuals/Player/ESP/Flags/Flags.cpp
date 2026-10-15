#include "Flags.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include "../../../../../../Utilities/Config/Config.hpp"
#include "../../../../../../External/ImGUI/imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

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
        ImFont* font = ImGui::GetFont();
        if (!font) return;

        const float fh = static_cast<float>(height);

        const bool wantFlashed = isEnemy ? config.esp_enemy_flags_flashed : config.esp_team_flags_flashed;
        const bool wantScoped = isEnemy ? config.esp_enemy_flags_scoped : config.esp_team_flags_scoped;
        const bool wantDefusing = isEnemy ? config.esp_enemy_flags_defusing : config.esp_team_flags_defusing;
        const bool wantPlanting = isEnemy ? config.esp_enemy_flags_planting : config.esp_team_flags_planting;
        const bool wantBomb = isEnemy ? config.esp_enemy_flags_bomb : config.esp_team_flags_bomb;
        const bool wantArmor = isEnemy ? config.esp_enemy_flags_armor : config.esp_team_flags_armor;
        const bool wantReload = isEnemy ? config.esp_enemy_flags_reloading : config.esp_team_flags_reloading;
        const bool wantMoney = isEnemy ? config.esp_enemy_flags_money : config.esp_team_flags_money;
        const bool wantPing = isEnemy ? config.esp_enemy_flags_ping : config.esp_team_flags_ping;
        const bool wantAmmo = isEnemy ? config.esp_enemy_flags_ammo : config.esp_team_flags_ammo;

        const float* colFlashed = isEnemy ? config.esp_enemy_flags_flashed_color : config.esp_team_flags_flashed_color;
        const float* colScoped = isEnemy ? config.esp_enemy_flags_scoped_color : config.esp_team_flags_scoped_color;
        const float* colDefusing = isEnemy ? config.esp_enemy_flags_defusing_color : config.esp_team_flags_defusing_color;
        const float* colPlanting = isEnemy ? config.esp_enemy_flags_planting_color : config.esp_team_flags_planting_color;
        const float* colBomb = isEnemy ? config.esp_enemy_flags_bomb_color : config.esp_team_flags_bomb_color;
        const float* colArmor = isEnemy ? config.esp_enemy_flags_armor_color : config.esp_team_flags_armor_color;
        const float* colReload = isEnemy ? config.esp_enemy_flags_reloading_color : config.esp_team_flags_reloading_color;
        const float* colMoney = isEnemy ? config.esp_enemy_flags_money_color : config.esp_team_flags_money_color;
        const float* colPing = isEnemy ? config.esp_enemy_flags_ping_color : config.esp_team_flags_ping_color;
        const float* colAmmo = isEnemy ? config.esp_enemy_flags_ammo_color : config.esp_team_flags_ammo_color;

        const ImU32 shadowCol = IM_COL32(0, 0, 0, 200);

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;

            Vector2 feetScreen, headScreen, centerScreen;
            if (!Math::WorldToScreen(ent.feetPos, feetScreen, viewMatrix, width, height)) continue;
            if (!Math::WorldToScreen(ent.headPos, headScreen, viewMatrix, width, height)) continue;

            const Vector3 centerWorld = ent.hasBones
                ? ent.bones[static_cast<int>(BoneIndex::Spine_1)]
                : ent.origin;

            const bool centerOk = Math::WorldToScreen(centerWorld, centerScreen, viewMatrix, width, height);

            if (!std::isfinite(feetScreen.y) || !std::isfinite(headScreen.y)) continue;

            const float centerX = centerOk ? centerScreen.x : feetScreen.x;
            if (!std::isfinite(centerX)) continue;

            const float rawH = feetScreen.y - headScreen.y;
            if (rawH < 1.0f) continue;

            const float closeT = (std::clamp)(rawH / (fh * 0.75f), 0.0f, 1.0f);

            const float kTopMul = 0.05f + closeT * 0.18f;
            const float kBotMul = 0.05f + closeT * 0.06f;
            const float kAspect = 0.45f + closeT * 0.30f;

            const float boxH = rawH * (1.0f + kTopMul + kBotMul);
            const float boxW = boxH * kAspect;
            const float topY = headScreen.y - rawH * kTopMul;
            const float right = centerX + boxW * 0.5f;

            struct Line { char text[48]; const float* col; };
            Line lines[12];
            int lineCount = 0;

            if (wantFlashed && ent.isFlashed) {
                snprintf(lines[lineCount].text, sizeof(lines[0].text), "Flashed");
                lines[lineCount].col = colFlashed;
                ++lineCount;
            }
            if (wantScoped && ent.isScoped) {
                snprintf(lines[lineCount].text, sizeof(lines[0].text), "Scoped");
                lines[lineCount].col = colScoped;
                ++lineCount;
            }
            if (wantDefusing && ent.isDefusing) {
                snprintf(lines[lineCount].text, sizeof(lines[0].text), "Defusing");
                lines[lineCount].col = colDefusing;
                ++lineCount;
            }

            const bool plantingNow = (wantPlanting && ent.isPlanting);
            if (plantingNow) {
                snprintf(lines[lineCount].text, sizeof(lines[0].text), "Planting");
                lines[lineCount].col = colPlanting;
                ++lineCount;
            }
            else if (wantBomb && ent.hasBomb) {
                snprintf(lines[lineCount].text, sizeof(lines[0].text), "Bomb");
                lines[lineCount].col = colBomb;
                ++lineCount;
            }

            if (wantArmor && ent.armor > 0) {
                if (ent.hasHelmet)
                    snprintf(lines[lineCount].text, sizeof(lines[0].text), "HK %d", ent.armor);
                else
                    snprintf(lines[lineCount].text, sizeof(lines[0].text), "K %d", ent.armor);
                lines[lineCount].col = colArmor;
                ++lineCount;
            }
            else if (wantArmor && ent.hasHelmet) {
                snprintf(lines[lineCount].text, sizeof(lines[0].text), "Helmet");
                lines[lineCount].col = colArmor;
                ++lineCount;
            }
            if (wantReload && ent.isReloading) {
                snprintf(lines[lineCount].text, sizeof(lines[0].text), "Reloading");
                lines[lineCount].col = colReload;
                ++lineCount;
            }
            if (wantMoney && ent.money > 0) {
                snprintf(lines[lineCount].text, sizeof(lines[0].text), "$%d", ent.money);
                lines[lineCount].col = colMoney;
                ++lineCount;
            }
            if (wantPing && ent.ping > 0) {
                snprintf(lines[lineCount].text, sizeof(lines[0].text), "%dms", ent.ping);
                lines[lineCount].col = colPing;
                ++lineCount;
            }
            if (wantAmmo) {
                if (ent.ammo > 0 || ent.reserve > 0) {
                    snprintf(lines[lineCount].text, sizeof(lines[0].text), "%d / %d", ent.ammo, ent.reserve);
                    lines[lineCount].col = colAmmo;
                    ++lineCount;
                }
            }

            if (lineCount == 0) continue;

            const float fontSize = (std::clamp)(rawH * 0.09f, 9.0f, 13.0f);
            const float lineH = fontSize + 1.0f;

            float x = right + 4.0f;
            float y = topY;

            for (int i = 0; i < lineCount; ++i) {
                const ImU32 textCol = Col(lines[i].col);
                dl->AddText(font, fontSize,
                    ImVec2(x + 1.0f, y + 1.0f), shadowCol, lines[i].text);
                dl->AddText(font, fontSize,
                    ImVec2(x, y), textCol, lines[i].text);
                y += lineH;
            }
        }
    }

    void RenderFlagsESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_flags)
            RenderSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_flags)
            RenderSide(frame, viewMatrix, width, height, false);
    }
}