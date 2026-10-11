#include "UI.hpp"
#include "UI_Internal.hpp"
#include "UI_Header.hpp"
#include "UI_Stars.hpp"
#include "UI_Cards.hpp"
#include "../../Utilities/Config/Config.hpp"
#include "../../External/ImGUI/imgui.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <Windows.h>
#include <shellapi.h>
#include <atomic>

#pragma comment(lib, "shell32.lib")

extern bool g_unload_requested;
extern std::atomic<bool> g_AllowMemoryWrite;

namespace UI {

    namespace Internal {
        int   g_activeTab = 0;
        float g_iconHover[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        float g_iconSel[5] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        int   g_activeSub = 0;
        float g_subHover[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float g_subSel[4] = { 1.0f, 0.0f, 0.0f, 0.0f };

        float g_espHover[2][24] = {};
        float g_espSel[2][24] = {};
        float g_espColorHover[2][24] = {};

        bool  g_espExpanded[2] = { false, false };
        float g_espExpandT[2] = { 0.0f, 0.0f };
        float g_espExpanderHover[2] = { 0.0f, 0.0f };

        bool  g_flagsExpanded[2] = { false, false };
        float g_flagsExpandT[2] = { 0.0f, 0.0f };
        float g_flagsExpanderHover[2] = { 0.0f, 0.0f };
    }

    static float  g_linearAnim = 0.0f;
    static ImVec2 g_menuPos = ImVec2(-1.0f, -1.0f);
    static bool   g_dragging = false;
    static ImVec2 g_dragDelta = ImVec2(0.0f, 0.0f);

    static float  g_safeModeHover = 0.0f;
    static float  g_unloadHover = 0.0f;
    static bool   g_requestSafeModeConfirm = false;
    static bool   g_requestUnloadConfirm = false;

    static constexpr float kMenuAnimDur = 0.10f;

    static float* GetEspColorPtr(int sub, int idx) {
        if (sub == 0) {
            switch (idx) {
            case 1:  return config.esp_enemy_box_color;
            case 4:  return config.esp_enemy_box_fill_color;
            case 6:  return config.esp_enemy_skeleton_color;
            case 7:  return config.esp_enemy_name_color;
            case 8:  return config.esp_enemy_healthbar_color;
            case 9:  return config.esp_enemy_distance_color;
            case 10: return config.esp_enemy_headcircle_color;
            case 11: return config.esp_enemy_snapline_color;
            case 12: return config.esp_enemy_viewdirection_color;
            case 13: return config.esp_enemy_weapon_color;
            case 14: return config.esp_enemy_flags_color;
            case 15: return config.esp_enemy_flags_flashed_color;
            case 16: return config.esp_enemy_flags_scoped_color;
            case 17: return config.esp_enemy_flags_defusing_color;
            case 18: return config.esp_enemy_flags_planting_color;
            case 19: return config.esp_enemy_flags_bomb_color;
            case 20: return config.esp_enemy_flags_armor_color;
            case 21: return config.esp_enemy_flags_reloading_color;
            case 22: return config.esp_enemy_flags_money_color;
            case 23: return config.esp_enemy_flags_ammo_color;
            default: return nullptr;
            }
        }
        else {
            switch (idx) {
            case 1:  return config.esp_team_box_color;
            case 4:  return config.esp_team_box_fill_color;
            case 6:  return config.esp_team_skeleton_color;
            case 7:  return config.esp_team_name_color;
            case 8:  return config.esp_team_healthbar_color;
            case 9:  return config.esp_team_distance_color;
            case 10: return config.esp_team_headcircle_color;
            case 11: return config.esp_team_snapline_color;
            case 12: return config.esp_team_viewdirection_color;
            case 13: return config.esp_team_weapon_color;
            case 14: return config.esp_team_flags_color;
            case 15: return config.esp_team_flags_flashed_color;
            case 16: return config.esp_team_flags_scoped_color;
            case 17: return config.esp_team_flags_defusing_color;
            case 18: return config.esp_team_flags_planting_color;
            case 19: return config.esp_team_flags_bomb_color;
            case 20: return config.esp_team_flags_armor_color;
            case 21: return config.esp_team_flags_reloading_color;
            case 22: return config.esp_team_flags_money_color;
            case 23: return config.esp_team_flags_ammo_color;
            default: return nullptr;
            }
        }
    }

    static void PushPickerStyle(float s) {
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f * s, 8.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f * s, 6.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 3.0f * s);

        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.10f, 0.13f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.741f, 0.690f, 1.0f, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.14f, 0.18f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.18f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.20f, 0.20f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.741f, 0.690f, 1.0f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.85f, 0.80f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.88f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.741f, 0.690f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.741f, 0.690f, 1.0f, 0.20f));
    }

    static void PopPickerStyle() {
        ImGui::PopStyleColor(10);
        ImGui::PopStyleVar(7);
    }

    static void CfgRefreshFiles() {
        configBrowser.files.clear();
        configBrowser.lastRefresh = ImGui::GetTime();

        namespace fs = std::filesystem;
        std::error_code ec;
        fs::create_directories(configBrowser.dir, ec);
        if (!fs::exists(configBrowser.dir, ec)) return;

        for (auto& entry : fs::directory_iterator(configBrowser.dir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;
            const auto& p = entry.path();
            if (p.extension() != ".ini") continue;
            configBrowser.files.push_back(p.stem().string());
        }

        std::sort(configBrowser.files.begin(), configBrowser.files.end());

        if (configBrowser.selected >= (int)configBrowser.files.size())
            configBrowser.selected = configBrowser.files.empty()
            ? -1 : (int)configBrowser.files.size() - 1;
    }

    static std::string CfgFullPath(const std::string& name) {
        return configBrowser.dir + "\\" + name + ".ini";
    }

    static bool CfgMatches(const std::string& name, const char* filter) {
        if (!filter || filter[0] == '\0') return true;
        std::string n = name;
        std::string f = filter;
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        std::transform(f.begin(), f.end(), f.begin(), ::tolower);
        return n.find(f) != std::string::npos;
    }

    static void CfgCardBg(ImDrawList* dl, ImVec2 min, ImVec2 max, float alpha) {
        const int a8 = (int)(alpha * 255.0f);
        const int borderA = (int)(alpha * 70.0f);
        dl->AddRectFilled(min, max, IM_COL32(26, 26, 34, a8), 6.0f);
        dl->AddRect(min, max, IM_COL32(189, 176, 255, borderA), 6.0f, 0, 1.0f);
    }

    static ImU32 CfgFade(int r, int g, int b, float a) {
        int A = (int)(a * 255.0f);
        if (A < 0) A = 0;
        if (A > 255) A = 255;
        return IM_COL32(r, g, b, A);
    }

    static void CfgStyleInput(float s) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f * s, 8.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.15f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.17f, 0.26f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.741f, 0.690f, 1.0f, 0.25f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.86f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.55f, 0.53f, 0.68f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.741f, 0.690f, 1.0f, 0.35f));
    }

    static void CfgUnstyleInput() {
        ImGui::PopStyleColor(7);
        ImGui::PopStyleVar(3);
    }

    static void CfgDrawListCard(ImDrawList* dl, ImVec2 min, ImVec2 max,
        float alpha, float s, ImFont* font, float labelFs, float dt)
    {
        CfgCardBg(dl, min, max, alpha);

        const float pad = 14.0f * s;

        if (configBrowser.lastRefresh <= 0.0)
            CfgRefreshFiles();

        if (font) {
            dl->AddText(font, labelFs * 1.05f,
                ImVec2(min.x + pad, min.y + pad - 2.0f * s),
                CfgFade(200, 195, 235, alpha), "Configurations");
        }

        const float titleH = labelFs * 1.05f + 6.0f * s;
        const float searchY = min.y + pad + titleH;
        const float searchH = 32.0f * s;
        const float searchW = (max.x - min.x) - pad * 2.0f;

        CfgStyleInput(s);
        ImGui::SetCursorScreenPos(ImVec2(min.x + pad, searchY));
        ImGui::SetNextItemWidth(searchW);
        ImGui::InputTextWithHint("##cfg_search", "Search...",
            configBrowser.search, sizeof(configBrowser.search));
        CfgUnstyleInput();

        const float listY = searchY + searchH + 10.0f * s;
        const float listH = (max.y - pad) - listY;
        if (listH <= 4.0f) return;

        ImGui::SetCursorScreenPos(ImVec2(min.x + pad, listY));

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f * s, 2.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 10.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 5.0f * s);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.08f, 0.08f, 0.11f, 0.55f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.741f, 0.690f, 1.0f, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.741f, 0.690f, 1.0f, 0.55f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.741f, 0.690f, 1.0f, 0.80f));

        ImGui::BeginChild("##cfg_list", ImVec2(searchW, listH), false,
            ImGuiWindowFlags_NoSavedSettings);

        ImDrawList* cdl = ImGui::GetWindowDrawList();

        const float rowH = 28.0f * s;
        const float rowGap = 4.0f * s;
        const float rowRadius = 5.0f * s;
        const float rowW = searchW - 12.0f * s;

        const float kApproach = 1.0f - expf(-18.0f * dt);

        int visibleIndex = 0;
        int clicked = -1;

        for (int i = 0; i < (int)configBrowser.files.size(); ++i) {
            if (!CfgMatches(configBrowser.files[i], configBrowser.search)) continue;
            if (visibleIndex >= 256) break;

            const ImVec2 rowMin = ImGui::GetCursorScreenPos();
            const ImVec2 rowMax(rowMin.x + rowW, rowMin.y + rowH);

            ImGui::PushID(i);
            ImGui::InvisibleButton("##row", ImVec2(rowW, rowH));
            ImGui::PopID();

            const bool hov = ImGui::IsItemHovered();
            const bool pressed = ImGui::IsItemClicked(0);

            configBrowser.rowHover[visibleIndex] +=
                ((hov ? 1.0f : 0.0f) - configBrowser.rowHover[visibleIndex]) * kApproach;

            const bool sel = (configBrowser.selected == i);

            if (sel) {
                cdl->AddRectFilled(rowMin, rowMax,
                    CfgFade(120, 105, 200, alpha * 0.55f), rowRadius);
                cdl->AddRect(rowMin, rowMax,
                    CfgFade(189, 176, 255, alpha * 0.75f), rowRadius, 0, 1.0f * s);
            }
            else if (configBrowser.rowHover[visibleIndex] > 0.005f) {
                const float hv = configBrowser.rowHover[visibleIndex];
                cdl->AddRectFilled(rowMin, rowMax,
                    CfgFade(120, 110, 180, alpha * 0.22f * hv), rowRadius);
            }

            if (font) {
                const float txtA = alpha * (0.72f + 0.28f * (sel ? 1.0f : configBrowser.rowHover[visibleIndex]));
                cdl->AddText(font, labelFs,
                    ImVec2(rowMin.x + 10.0f * s, rowMin.y + (rowH - labelFs) * 0.5f),
                    CfgFade(210, 205, 245, txtA), configBrowser.files[i].c_str());
            }

            if (pressed) clicked = i;

            ImGui::SetCursorScreenPos(ImVec2(rowMin.x, rowMax.y + rowGap));

            visibleIndex++;
        }

        ImGui::Dummy(ImVec2(searchW, 4.0f * s));

        if (clicked >= 0) {
            if (configBrowser.selected != clicked) Internal::PlayClickSound();
            configBrowser.selected = clicked;
        }

        ImGui::EndChild();
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(5);
    }

    struct CfgBtnDef { const char* icon; const char* label; };

    static const CfgBtnDef kCfgButtons[6] = {
        { "\xEE\x9C\x90", "New"     },
        { "\xEE\x9D\x8E", "Save"    },
        { "\xEE\xA3\xA5", "Load"    },
        { "\xEE\x9D\x8D", "Delete"  },
        { "\xEE\x9C\xAC", "Refresh" },
        { "\xEE\xA2\xB7", "Folder"  },
    };

    static void CfgBigButton(ImDrawList* dl, ImVec2 min, ImVec2 max,
        const CfgBtnDef& def, float hover, float alpha, float s,
        ImFont* textFont, float labelFs, ImFont* iconFont)
    {
        const float radius = 8.0f * s;
        const int a8 = (int)(alpha * 255.0f);
        const int baseR = (int)(30 + (56 - 30) * hover);
        const int baseG = (int)(30 + (52 - 30) * hover);
        const int baseB = (int)(40 + (78 - 40) * hover);

        dl->AddRectFilled(min, max, IM_COL32(baseR, baseG, baseB, a8), radius);

        const int brR = (int)(120 + (189 - 120) * hover);
        const int brG = (int)(110 + (176 - 110) * hover);
        const int brB = (int)(180 + (255 - 180) * hover);
        const int brA = (int)(alpha * (0.35f + 0.55f * hover) * 255.0f);
        dl->AddRect(min, max, IM_COL32(brR, brG, brB, brA), radius, 0, 1.0f * s);

        const int tR = (int)(200 + (245 - 200) * hover);
        const int tG = (int)(195 + (240 - 195) * hover);
        const int tB = (int)(230 + (255 - 230) * hover);
        const ImU32 txtCol = IM_COL32(tR, tG, tB, (int)(alpha * 255.0f));

        if (iconFont) {
            const float iconSz = (max.y - min.y) * 0.34f;
            const ImVec2 sz = iconFont->CalcTextSizeA(iconSz, FLT_MAX, 0.0f, def.icon);
            const float tx = (min.x + max.x) * 0.5f - sz.x * 0.5f;
            const float ty = (min.y + max.y) * 0.5f - sz.y * 0.5f;
            dl->AddText(iconFont, iconSz,
                ImVec2(floorf(tx + 0.5f), floorf(ty + 0.5f)),
                txtCol, def.icon);
        }
        else if (textFont) {
            const ImVec2 sz = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, def.label);
            const float tx = (min.x + max.x) * 0.5f - sz.x * 0.5f;
            const float ty = (min.y + max.y) * 0.5f - sz.y * 0.5f;
            dl->AddText(textFont, labelFs,
                ImVec2(floorf(tx + 0.5f), floorf(ty + 0.5f)),
                txtCol, def.label);
        }
    }

    static int  s_confirmAction = -1;
    static char s_confirmName[128] = {};

    static void CfgExecuteAction(int action, const std::string& name) {
        switch (action) {
        case 1:
            config.Save(CfgFullPath(name));
            break;
        case 2:
            config.Load(CfgFullPath(name));
            break;
        case 3: {
            std::error_code ec;
            std::filesystem::remove(CfgFullPath(name), ec);
            CfgRefreshFiles();
            break;
        }
        }
    }

    static void CfgDrawActionsCard(ImDrawList* dl, ImVec2 min, ImVec2 max,
        float alpha, float s, ImFont* textFont, float labelFs, float dt)
    {
        CfgCardBg(dl, min, max, alpha);

        const float pad = 14.0f * s;

        if (textFont) {
            dl->AddText(textFont, labelFs * 1.05f,
                ImVec2(min.x + pad, min.y + pad - 2.0f * s),
                CfgFade(200, 195, 235, alpha), "Actions");
        }

        ImFont* iconFont = Internal::GetIconFont();

        const float titleH = labelFs * 1.05f + 6.0f * s;
        const float gridY = min.y + pad + titleH + 6.0f * s;
        const float gridX = min.x + pad;
        const float gridW = (max.x - min.x) - pad * 2.0f;
        const float gridH = (max.y - pad) - gridY;

        const float gap = 8.0f * s;
        const float btnW = (gridW - gap * 2.0f) / 3.0f;
        const float btnH = (gridH - gap) / 2.0f;

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const float kApproach = 1.0f - expf(-18.0f * dt);

        const bool confirmActive = ImGui::IsPopupOpen("##cfg_confirm");
        const bool createActive = ImGui::IsPopupOpen("##cfg_create");
        const bool blockButtons = confirmActive || createActive;

        int clickedBtn = -1;

        for (int i = 0; i < 6; ++i) {
            const int col = i % 3;
            const int row = i / 3;

            const ImVec2 bmin(
                gridX + col * (btnW + gap),
                gridY + row * (btnH + gap));
            const ImVec2 bmax(bmin.x + btnW, bmin.y + btnH);

            const bool hov = !blockButtons
                && mouse.x >= bmin.x && mouse.x <= bmax.x
                && mouse.y >= bmin.y && mouse.y <= bmax.y;

            configBrowser.btnHover[i] +=
                ((hov ? 1.0f : 0.0f) - configBrowser.btnHover[i]) * kApproach;

            CfgBigButton(dl, bmin, bmax, kCfgButtons[i],
                configBrowser.btnHover[i], alpha, s, textFont, labelFs, iconFont);

            if (hov && ImGui::IsMouseClicked(0)) clickedBtn = i;
        }

        if (clickedBtn >= 0) {
            Internal::PlayClickSound();

            const bool hasSel =
                configBrowser.selected >= 0 &&
                configBrowser.selected < (int)configBrowser.files.size();

            if (clickedBtn == 0) {
                configBrowser.newName[0] = '\0';
                ImGui::OpenPopup("##cfg_create");
            }
            else if (clickedBtn == 4) {
                CfgRefreshFiles();
            }
            else if (clickedBtn == 5) {
                ShellExecuteA(nullptr, "open", configBrowser.dir.c_str(),
                    nullptr, nullptr, SW_SHOW);
            }
            else if (hasSel) {
                s_confirmAction = clickedBtn;
                snprintf(s_confirmName, sizeof(s_confirmName), "%s",
                    configBrowser.files[configBrowser.selected].c_str());
                ImGui::OpenPopup("##cfg_confirm");
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 7.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f * s, 10.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(9.0f * s, 6.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f * s, 7.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));

        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.10f, 0.13f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.741f, 0.690f, 1.0f, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.10f, 0.10f, 0.13f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.10f, 0.10f, 0.13f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.10f, 0.10f, 0.13f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.15f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.17f, 0.26f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.86f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.18f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.25f, 0.38f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.30f, 0.48f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.45f));

        const ImVec2 center(
            min.x + (max.x - min.x) * 0.5f,
            min.y + (max.y - min.y) * 0.5f);

        const ImGuiWindowFlags modalFlags =
            ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("##cfg_create", nullptr, modalFlags)) {
            if (textFont)
                ImGui::TextUnformatted("New configuration");
            ImGui::SetNextItemWidth(180.0f * s);
            ImGui::InputTextWithHint("##cfg_name", "name...",
                configBrowser.newName, sizeof(configBrowser.newName));

            const bool valid = configBrowser.newName[0] != '\0';

            ImGui::BeginDisabled(!valid);
            if (ImGui::Button("Create", ImVec2(80.0f * s, 0))) {
                config.Save(CfgFullPath(std::string(configBrowser.newName)));
                CfgRefreshFiles();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(80.0f * s, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("##cfg_confirm", nullptr, modalFlags)) {
            const char* verb =
                (s_confirmAction == 1) ? "overwrite" :
                (s_confirmAction == 2) ? "load" :
                "delete";

            if (textFont)
                ImGui::Text("Are you sure you would like to %s", verb);
            if (textFont)
                ImGui::Text("'%s' ?", s_confirmName);

            ImGui::Spacing();

            if (ImGui::Button("Yes", ImVec2(80.0f * s, 0))) {
                CfgExecuteAction(s_confirmAction, s_confirmName);
                s_confirmAction = -1;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(80.0f * s, 0))) {
                s_confirmAction = -1;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        else {
            s_confirmAction = -1;
        }

        ImGui::PopStyleColor(13);
        ImGui::PopStyleVar(7);
    }

    static void DrawSystemCard(ImDrawList* dl, ImVec2 pos, float menuW, float menuH,
        float hdrH, float s, float a, float dt, ImFont* font, float labelFs)
    {
        const float pad = 16.0f * s;
        const float gap = 12.0f * s;

        const float contentX = pos.x + pad;
        const float contentY = pos.y + hdrH + pad;
        const float contentW = menuW - pad * 2.0f;
        const float contentH = menuH - hdrH - pad * 2.0f;

        const float cardW = (contentW - gap * 2.0f) / 3.0f;
        const float cardH = contentH;

        const ImVec2 cmin(contentX, contentY);
        const ImVec2 cmax(contentX + cardW, contentY + cardH);

        const float cardAlpha = a * s;

        CfgCardBg(dl, cmin, cmax, cardAlpha);

        const float cp = 14.0f * s;
        const float rowH = 22.0f * s;

        const float cbSize = 18.0f * s;
        const float cbX1 = cmax.x - cp;
        const float cbX0 = cbX1 - cbSize;
        const float rowY = cmin.y + cp;
        const float cbY0 = rowY + (rowH - cbSize) * 0.5f;
        const float cbY1 = cbY0 + cbSize;

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const bool interactive = (a > 0.99f);

        const bool safeCbHovered = interactive
            && mouse.x >= cbX0 && mouse.x <= cbX1
            && mouse.y >= cbY0 && mouse.y <= cbY1;

        const float kApproach = 1.0f - expf(-18.0f * dt);
        g_safeModeHover += ((safeCbHovered ? 1.0f : 0.0f) - g_safeModeHover) * kApproach;

        const bool safeEnabled = config.safe_mode;
        const float safeSelT = safeEnabled ? 1.0f : 0.0f;
        const float safeSt = (safeSelT > g_safeModeHover) ? safeSelT : g_safeModeHover;

        const int mR = (int)(120 + (189 - 120) * safeSt);
        const int mG = (int)(110 + (176 - 110) * safeSt);
        const int mB = (int)(180 + (255 - 180) * safeSt);
        const int mFillA = (int)(cardAlpha * (0.35f + 0.65f * safeSelT) * 255.0f);
        const int mRingA = (int)(cardAlpha * 0.55f * g_safeModeHover * 255.0f);

        dl->AddRectFilled(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
            IM_COL32(mR, mG, mB, mFillA), 4.0f * s);

        if (mRingA > 2) {
            dl->AddRect(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
                IM_COL32(mR, mG, mB, mRingA), 4.0f * s, 0, 1.5f * s);
        }

        if (font) {
            const char* lbl = "Safemode";
            const ImVec2 lsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, lbl);
            const float lx = cmin.x + cp;
            const float ly = rowY + (rowH - lsz.y) * 0.5f;

            const int lr = (int)(190 + (235 - 190) * safeSt);
            const int lg = (int)(185 + (228 - 185) * safeSt);
            const int lb = (int)(215 + (255 - 215) * safeSt);

            dl->AddText(font, labelFs,
                ImVec2(floorf(lx + 0.5f), floorf(ly + 0.5f)),
                IM_COL32(lr, lg, lb, (int)(cardAlpha * 255.0f)), lbl);
        }

        if (safeCbHovered && ImGui::IsMouseClicked(0)) {
            Internal::PlayClickSound();
            if (safeEnabled) {
                if (config.safe_mode_no_warn)
                    config.safe_mode = false;
                else
                    g_requestSafeModeConfirm = true;
            }
            else {
                config.safe_mode = true;
            }
        }

        const float btnH = 34.0f * s;
        const float btnY = cmax.y - cp - btnH;
        const float btnX0 = cmin.x + cp;
        const float btnX1 = cmax.x - cp;

        const bool unloadHovered = interactive
            && mouse.x >= btnX0 && mouse.x <= btnX1
            && mouse.y >= btnY && mouse.y <= btnY + btnH;

        g_unloadHover += ((unloadHovered ? 1.0f : 0.0f) - g_unloadHover) * kApproach;

        const int uR = (int)(60 + (180 - 60) * g_unloadHover);
        const int uG = (int)(24 + (55 - 24) * g_unloadHover);
        const int uB = (int)(30 + (65 - 30) * g_unloadHover);

        dl->AddRectFilled(ImVec2(btnX0, btnY), ImVec2(btnX1, btnY + btnH),
            IM_COL32(uR, uG, uB, (int)(cardAlpha * 255.0f)), 5.0f * s);
        dl->AddRect(ImVec2(btnX0, btnY), ImVec2(btnX1, btnY + btnH),
            IM_COL32(200, 80, 90, (int)(cardAlpha * (0.40f + 0.50f * g_unloadHover) * 255.0f)),
            5.0f * s, 0, 1.0f * s);

        if (font) {
            const ImVec2 bsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, "Unload");
            const float bx = (btnX0 + btnX1) * 0.5f - bsz.x * 0.5f;
            const float by = btnY + (btnH - bsz.y) * 0.5f;
            dl->AddText(font, labelFs,
                ImVec2(floorf(bx + 0.5f), floorf(by + 0.5f)),
                IM_COL32(245, 225, 230, (int)(cardAlpha * 255.0f)), "Unload");
        }

        if (unloadHovered && ImGui::IsMouseClicked(0)) {
            Internal::PlayClickSound();
            g_requestUnloadConfirm = true;
        }
    }

    static void PushWarningPopupStyle(float s) {
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f * s, 14.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f * s, 7.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f * s, 9.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));

        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.10f, 0.10f, 0.13f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.741f, 0.690f, 1.0f, 0.42f));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.10f, 0.10f, 0.13f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.10f, 0.10f, 0.13f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.10f, 0.10f, 0.13f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.15f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.17f, 0.26f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.86f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.18f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.25f, 0.38f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.30f, 0.48f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ModalWindowDimBg, ImVec4(0.0f, 0.0f, 0.0f, 0.55f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.741f, 0.690f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.741f, 0.690f, 1.0f, 0.22f));
    }

    static void PopWarningPopupStyle() {
        ImGui::PopStyleColor(15);
        ImGui::PopStyleVar(8);
    }

    static void DrawPopupTitle(ImVec4 color, const char* title) {
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::Spacing();
    }

    static void DrawSafeModeConfirmPopup(ImVec2 pos, float menuW, float menuH, float s)
    {
        if (g_requestSafeModeConfirm) {
            ImGui::OpenPopup("##safe_mode_confirm");
            g_requestSafeModeConfirm = false;
        }

        PushWarningPopupStyle(s);

        const ImVec2 center(
            pos.x + menuW * 0.5f,
            pos.y + menuH * 0.5f);

        const ImGuiWindowFlags modalFlags =
            ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("##safe_mode_confirm", nullptr, modalFlags)) {
            DrawPopupTitle(ImVec4(1.0f, 0.78f, 0.55f, 1.0f), "Warning");

            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 300.0f * s);
            ImGui::TextUnformatted(
                "Disabling safemode allows the cheat to modify game memory. "
                "This can trigger VAC or cause game instability. "
                "Only disable safemode if you understand the risks.");
            ImGui::PopTextWrapPos();

            ImGui::Spacing();

            if (ImGui::Checkbox("Do not remind me again",
                &config.safe_mode_no_warn))
            {
                Internal::PlayClickSound();
            }

            ImGui::Spacing();

            if (ImGui::Button("Disable", ImVec2(96.0f * s, 0))) {
                config.safe_mode = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(96.0f * s, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        PopWarningPopupStyle();
    }

    static void DrawUnloadConfirmPopup(ImVec2 pos, float menuW, float menuH, float s)
    {
        if (g_requestUnloadConfirm) {
            ImGui::OpenPopup("##unload_confirm");
            g_requestUnloadConfirm = false;
        }

        PushWarningPopupStyle(s);

        const ImVec2 center(
            pos.x + menuW * 0.5f,
            pos.y + menuH * 0.5f);

        const ImGuiWindowFlags modalFlags =
            ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("##unload_confirm", nullptr, modalFlags)) {
            DrawPopupTitle(ImVec4(1.0f, 0.72f, 0.72f, 1.0f), "Unload");

            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 260.0f * s);
            ImGui::TextUnformatted("Are you sure you want to unload?");
            ImGui::PopTextWrapPos();

            ImGui::Spacing();

            if (ImGui::Button("Unload", ImVec2(96.0f * s, 0))) {
                g_unload_requested = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(96.0f * s, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        PopWarningPopupStyle();
    }

    void Render(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height) {
        (void)device; (void)context; (void)width; (void)height;

        g_AllowMemoryWrite.store(!config.safe_mode, std::memory_order_relaxed);

        const ImGuiIO& io = ImGui::GetIO();

        float dt = io.DeltaTime;
        if (dt > 0.05f) dt = 0.05f;
        if (dt < 0.0f)  dt = 0.0f;

        const bool open = config.show_menu;

        const float step = (kMenuAnimDur > 0.0f) ? (dt / kMenuAnimDur) : 1.0f;
        if (open) g_linearAnim = (std::min)(1.0f, g_linearAnim + step);
        else      g_linearAnim = (std::max)(0.0f, g_linearAnim - step);

        if (g_linearAnim < 0.001f && !open) {
            g_dragging = false;
            return;
        }

        const float raw = g_linearAnim;
        const float a = raw * raw * (3.0f - 2.0f * raw);

        constexpr float kBaseW = 720.0f;
        constexpr float kBaseH = 460.0f;
        constexpr float kHeaderH = 46.0f;
        constexpr float kSmallScale = 0.10f;

        const float s = kSmallScale + (1.0f - kSmallScale) * a;
        const float menuW = kBaseW * s;
        const float menuH = kBaseH * s;

        if (g_menuPos.x < 0.0f || g_menuPos.y < 0.0f) {
            g_menuPos.x = io.DisplaySize.x * 0.5f - kBaseW * 0.5f;
            g_menuPos.y = io.DisplaySize.y * 0.5f - kBaseH * 0.5f;
        }

        const float anchorX = g_menuPos.x + kBaseW * 0.5f;
        const float finalBottomY = g_menuPos.y + kBaseH;
        const float startBottomY = io.DisplaySize.y;
        const float bottomY = startBottomY + (finalBottomY - startBottomY) * a;

        const float drawX = anchorX - menuW * 0.5f;
        const float drawY = bottomY - menuH;

        ImGui::SetNextWindowSize(ImVec2(menuW, menuH), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(drawX, drawY), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.075f, 0.075f, 0.090f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.741f, 0.690f, 1.0f, 0.35f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, a);

        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("##oblivion_menu", nullptr, flags);

        const int a255 = (int)(a * 255.0f);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 pos = ImGui::GetWindowPos();

        const float hdrH = kHeaderH * s;

        dl->AddRectFilled(
            pos, ImVec2(pos.x + menuW, pos.y + hdrH),
            IM_COL32(18, 18, 24, a255),
            6.0f, ImDrawFlags_RoundCornersTop);

        const float time = (float)ImGui::GetTime();

        if (a > 0.5f) {
            const float starAreaY = pos.y + hdrH;
            const float starAreaH = menuH - hdrH;

            dl->PushClipRect(
                ImVec2(pos.x, starAreaY),
                ImVec2(pos.x + menuW, pos.y + menuH),
                true);

            Stars::Draw(dl,
                ImVec2(pos.x, starAreaY),
                ImVec2(pos.x + menuW, starAreaY + starAreaH),
                time, a * s);

            dl->PopClipRect();
        }

        bool anyIconHover = false;
        Header::Draw(dl, pos, menuW, hdrH, s, a, dt, anyIconHover);

        const bool showEspCard = (a > 0.5f)
            && Internal::g_activeTab == 1
            && (Internal::g_activeSub == 0 || Internal::g_activeSub == 1);

        const bool showConfigCards = (a > 0.5f) && Internal::g_activeTab == 3;
        const bool showSystemCard = (a > 0.5f) && Internal::g_activeTab == 4;

        ImFont* font = io.FontDefault ? io.FontDefault : ImGui::GetFont();
        const float labelFs = 15.0f * s;

        if (showEspCard) {
            const float pad = 16.0f * s;
            const float gap = 12.0f * s;

            const float contentX = pos.x + pad;
            const float contentY = pos.y + hdrH + pad;
            const float contentW = menuW - pad * 2.0f;
            const float contentH = menuH - hdrH - pad * 2.0f;

            const float cardW = (contentW - gap * 2.0f) / 3.0f;
            const float cardH = contentH;

            const float contentAlpha = a * s;

            const ImVec2 c0min(contentX, contentY);
            const ImVec2 c0max(contentX + cardW, contentY + cardH);

            const int curSub = Internal::g_activeSub;

            bool* vals[Cards::kEspItemCount];
            vals[0] = (curSub == 0) ? &config.esp_enemy_enable : &config.esp_team_enable;
            vals[1] = (curSub == 0) ? &config.esp_enemy_box : &config.esp_team_box;
            vals[2] = (curSub == 0) ? &config.esp_enemy_box_style_corner : &config.esp_team_box_style_corner;
            vals[3] = (curSub == 0) ? &config.esp_enemy_box_style_rounded : &config.esp_team_box_style_rounded;
            vals[4] = (curSub == 0) ? &config.esp_enemy_box_style_fill : &config.esp_team_box_style_fill;
            vals[5] = (curSub == 0) ? &config.esp_enemy_box_outline : &config.esp_team_box_outline;
            vals[6] = (curSub == 0) ? &config.esp_enemy_skeleton : &config.esp_team_skeleton;
            vals[7] = (curSub == 0) ? &config.esp_enemy_name : &config.esp_team_name;
            vals[8] = (curSub == 0) ? &config.esp_enemy_healthbar : &config.esp_team_healthbar;
            vals[9] = (curSub == 0) ? &config.esp_enemy_distance : &config.esp_team_distance;
            vals[10] = (curSub == 0) ? &config.esp_enemy_headcircle : &config.esp_team_headcircle;
            vals[11] = (curSub == 0) ? &config.esp_enemy_snapline : &config.esp_team_snapline;
            vals[12] = (curSub == 0) ? &config.esp_enemy_viewdirection : &config.esp_team_viewdirection;
            vals[13] = (curSub == 0) ? &config.esp_enemy_weapon : &config.esp_team_weapon;
            vals[14] = (curSub == 0) ? &config.esp_enemy_flags : &config.esp_team_flags;
            vals[15] = (curSub == 0) ? &config.esp_enemy_flags_flashed : &config.esp_team_flags_flashed;
            vals[16] = (curSub == 0) ? &config.esp_enemy_flags_scoped : &config.esp_team_flags_scoped;
            vals[17] = (curSub == 0) ? &config.esp_enemy_flags_defusing : &config.esp_team_flags_defusing;
            vals[18] = (curSub == 0) ? &config.esp_enemy_flags_planting : &config.esp_team_flags_planting;
            vals[19] = (curSub == 0) ? &config.esp_enemy_flags_bomb : &config.esp_team_flags_bomb;
            vals[20] = (curSub == 0) ? &config.esp_enemy_flags_armor : &config.esp_team_flags_armor;
            vals[21] = (curSub == 0) ? &config.esp_enemy_flags_reloading : &config.esp_team_flags_reloading;
            vals[22] = (curSub == 0) ? &config.esp_enemy_flags_money : &config.esp_team_flags_money;
            vals[23] = (curSub == 0) ? &config.esp_enemy_flags_ammo : &config.esp_team_flags_ammo;

            const float expandBox = Internal::g_espExpandT[curSub];
            const float expandFlags = Internal::g_flagsExpandT[curSub];

            const ImVec2 mouse = io.MousePos;
            const float kApproach = 1.0f - expf(-18.0f * dt);

            // Card background + border drawn on parent list so it stays behind scrolled content
            {
                const int bgA = (int)(contentAlpha * 255.0f);
                const int borderA = (int)(contentAlpha * 70.0f);
                dl->AddRectFilled(c0min, c0max, IM_COL32(26, 26, 34, bgA), 6.0f);
                dl->AddRect(c0min, c0max, IM_COL32(189, 176, 255, borderA),
                    6.0f, 0, 1.0f);
            }

            ImGui::SetCursorScreenPos(c0min);

            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f * s);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 8.0f * s);
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 4.0f * s);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.08f, 0.08f, 0.11f, 0.55f));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.741f, 0.690f, 1.0f, 0.35f));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.741f, 0.690f, 1.0f, 0.55f));
            ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.741f, 0.690f, 1.0f, 0.80f));

            ImGui::BeginChild("##esp_scroll", ImVec2(cardW, cardH), false,
                ImGuiWindowFlags_NoSavedSettings);

            // Reserve scrollable content height based on visible rows.
            constexpr float kRowH = 24.0f;
            constexpr float kTopOff = 8.0f;
            const float totalRows = 2.0f + 4.0f * expandBox + 9.0f + 9.0f * expandFlags;
            const float contentHeight = (kTopOff + totalRows * kRowH + 14.0f) * s;
            ImGui::Dummy(ImVec2(cardW, contentHeight));

            const float scrollY = ImGui::GetScrollY();
            const ImVec2 drawMin(c0min.x, c0min.y - scrollY);
            const ImVec2 drawMax(c0max.x, c0max.y - scrollY);

            ImDrawList* cdl = ImGui::GetWindowDrawList();
            cdl->PushClipRect(c0min, c0max, true);

            const bool mouseInCard = (a > 0.99f)
                && mouse.x >= c0min.x && mouse.x <= c0max.x
                && mouse.y >= c0min.y && mouse.y <= c0max.y;

            for (int i = 0; i < Cards::kEspItemCount; ++i) {
                const float rowA = Cards::RowAlpha(i, expandBox, expandFlags);
                const bool interactive = mouseInCard && (rowA > 0.5f);

                Cards::Rect cb = Cards::GetCheckboxRect(
                    drawMin, drawMax, i, expandBox, expandFlags, s);

                const bool hov = interactive
                    && mouse.x >= cb.min.x && mouse.x <= cb.max.x
                    && mouse.y >= cb.min.y && mouse.y <= cb.max.y;

                if (hov && ImGui::IsMouseClicked(0)) {
                    Internal::PlayClickSound();
                    *(vals[i]) = !(*(vals[i]));
                }

                Internal::g_espHover[curSub][i] +=
                    ((hov ? 1.0f : 0.0f) - Internal::g_espHover[curSub][i]) * kApproach;
                Internal::g_espSel[curSub][i] +=
                    (((*(vals[i])) ? 1.0f : 0.0f) - Internal::g_espSel[curSub][i]) * kApproach;
            }

            static char s_popupId[Cards::kEspItemCount][32] = {};

            for (int i = 0; i < Cards::kEspItemCount; ++i) {
                const float rowA = Cards::RowAlpha(i, expandBox, expandFlags);
                const bool interactive = mouseInCard && (rowA > 0.5f);

                bool colorHov = false;

                float* colorPtr = Cards::HasColor(i) ? GetEspColorPtr(curSub, i) : nullptr;

                if (colorPtr) {
                    Cards::Rect cr = Cards::GetColorRect(
                        drawMin, drawMax, i, expandBox, expandFlags, s);

                    colorHov = interactive
                        && mouse.x >= cr.min.x && mouse.x <= cr.max.x
                        && mouse.y >= cr.min.y && mouse.y <= cr.max.y;

                    if (!s_popupId[i][0]) {
                        snprintf(s_popupId[i], sizeof(s_popupId[i]),
                            "##esp_color_%d", i);
                    }

                    if (colorHov && ImGui::IsMouseClicked(0)) {
                        Internal::PlayClickSound();
                        ImGui::OpenPopup(s_popupId[i]);
                    }

                    PushPickerStyle(s);
                    ImGui::PushItemWidth(150.0f * s);
                    if (ImGui::BeginPopup(s_popupId[i])) {
                        ImGui::ColorPicker4("##picker", colorPtr,
                            ImGuiColorEditFlags_AlphaBar
                            | ImGuiColorEditFlags_AlphaPreviewHalf
                            | ImGuiColorEditFlags_PickerHueBar
                            | ImGuiColorEditFlags_NoSidePreview
                            | ImGuiColorEditFlags_NoInputs);
                        ImGui::EndPopup();
                    }
                    ImGui::PopItemWidth();
                    PopPickerStyle();
                }

                Internal::g_espColorHover[curSub][i] +=
                    ((colorHov ? 1.0f : 0.0f) - Internal::g_espColorHover[curSub][i]) * kApproach;
            }

            Cards::Rect ex = Cards::GetExpanderRect(drawMin, drawMax, s,
                expandBox, expandFlags, font, labelFs);
            const bool exHov = mouseInCard
                && mouse.x >= ex.min.x && mouse.x <= ex.max.x
                && mouse.y >= ex.min.y && mouse.y <= ex.max.y;

            if (exHov && ImGui::IsMouseClicked(0)) {
                Internal::PlayClickSound();
                Internal::g_espExpanded[curSub] = !Internal::g_espExpanded[curSub];
            }

            Internal::g_espExpanderHover[curSub] +=
                ((exHov ? 1.0f : 0.0f) - Internal::g_espExpanderHover[curSub]) * kApproach;

            const float targetBox = Internal::g_espExpanded[curSub] ? 1.0f : 0.0f;
            Internal::g_espExpandT[curSub] +=
                (targetBox - Internal::g_espExpandT[curSub]) * kApproach;

            Cards::Rect fex = Cards::GetFlagsExpanderRect(drawMin, drawMax, s,
                expandBox, font, labelFs);
            const bool fexHov = mouseInCard
                && mouse.x >= fex.min.x && mouse.x <= fex.max.x
                && mouse.y >= fex.min.y && mouse.y <= fex.max.y;

            if (fexHov && ImGui::IsMouseClicked(0)) {
                Internal::PlayClickSound();
                Internal::g_flagsExpanded[curSub] = !Internal::g_flagsExpanded[curSub];
            }

            Internal::g_flagsExpanderHover[curSub] +=
                ((fexHov ? 1.0f : 0.0f) - Internal::g_flagsExpanderHover[curSub]) * kApproach;

            const float targetFlags = Internal::g_flagsExpanded[curSub] ? 1.0f : 0.0f;
            Internal::g_flagsExpandT[curSub] +=
                (targetFlags - Internal::g_flagsExpandT[curSub]) * kApproach;

            const float* colorPtrs[Cards::kEspItemCount] = {};
            for (int i = 0; i < Cards::kEspItemCount; ++i) {
                if (Cards::HasColor(i))
                    colorPtrs[i] = GetEspColorPtr(curSub, i);
            }

            Cards::Draw(cdl, drawMin, drawMax, contentAlpha,
                Internal::g_espHover[curSub],
                Internal::g_espSel[curSub],
                Internal::g_espColorHover[curSub],
                colorPtrs,
                Internal::g_espExpandT[curSub],
                Internal::g_espExpanderHover[curSub],
                Internal::g_flagsExpandT[curSub],
                Internal::g_flagsExpanderHover[curSub],
                font, labelFs, s,
                false);

            cdl->PopClipRect();

            ImGui::EndChild();
            ImGui::PopStyleColor(5);
            ImGui::PopStyleVar(5);
        }
        else if (showConfigCards) {
            const float pad = 16.0f * s;
            const float gap = 12.0f * s;

            const float contentX = pos.x + pad;
            const float contentY = pos.y + hdrH + pad;
            const float contentW = menuW - pad * 2.0f;
            const float contentH = menuH - hdrH - pad * 2.0f;

            const float listW = (contentW - gap) * 0.60f;
            const float actionsW = (contentW - gap) - listW;

            const float contentAlpha = a * s;

            const ImVec2 listMin(contentX, contentY);
            const ImVec2 listMax(contentX + listW, contentY + contentH);

            const ImVec2 actMin(contentX + listW + gap, contentY);
            const ImVec2 actMax(actMin.x + actionsW, contentY + contentH);

            CfgDrawListCard(dl, listMin, listMax,
                contentAlpha, s, font, labelFs, dt);

            CfgDrawActionsCard(dl, actMin, actMax,
                contentAlpha, s, font, labelFs, dt);
        }
        else if (showSystemCard) {
            DrawSystemCard(dl, pos, menuW, menuH, hdrH, s, a, dt, font, labelFs);
        }

        DrawSafeModeConfirmPopup(pos, menuW, menuH, s);
        DrawUnloadConfirmPopup(pos, menuW, menuH, s);

        dl->AddRect(pos, ImVec2(pos.x + menuW, pos.y + menuH),
            IM_COL32(189, 176, 255, (int)(a * 90.0f)),
            6.0f, 0, 1.0f);

        if (a > 0.99f) {
            const ImVec2 mouse = io.MousePos;
            const bool inHeader =
                mouse.x >= pos.x && mouse.x <= pos.x + menuW &&
                mouse.y >= pos.y && mouse.y <= pos.y + hdrH;

            if (!g_dragging && ImGui::IsMouseClicked(0) && inHeader && !anyIconHover) {
                g_dragging = true;
                g_dragDelta = ImVec2(mouse.x - g_menuPos.x, mouse.y - g_menuPos.y);
            }

            if (g_dragging) {
                if (ImGui::IsMouseDown(0)) {
                    g_menuPos.x = mouse.x - g_dragDelta.x;
                    g_menuPos.y = mouse.y - g_dragDelta.y;

                    const float maxX = (std::max)(0.0f, io.DisplaySize.x - kBaseW);
                    const float maxY = (std::max)(0.0f, io.DisplaySize.y - kBaseH);

                    g_menuPos.x = (std::clamp)(g_menuPos.x, 0.0f, maxX);
                    g_menuPos.y = (std::clamp)(g_menuPos.y, 0.0f, maxY);
                }
                else {
                    g_dragging = false;
                }
            }
        }
        else {
            g_dragging = false;
        }

        ImGui::End();

        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor(2);
    }

    bool IsOpen() { return config.show_menu; }
    void Toggle() {}
}