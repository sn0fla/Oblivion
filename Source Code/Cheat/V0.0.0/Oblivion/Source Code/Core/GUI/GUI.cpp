#include "GUI.hpp"
#include "Framework/Framework.hpp"
#include "Widgets/Widgets.hpp"
#include "../../SDK/Entities/Entities.hpp"
#include "../../Utilities/Config/Config.hpp"

#include <imgui.h>
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

namespace GUI {

    int   g_activeTab = 0;
    float g_iconHover[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float g_iconSel[5] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    int   g_activeSub[5] = { 0, 0, 0, 0, 0 };
    float g_subHover[5][8] = {};
    float g_subSel[5][8] = {
        { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
    };

    float g_espHover[2][Widgets::kEspItemCount] = {};
    float g_espSel[2][Widgets::kEspItemCount] = {};
    float g_espColorHover[2][Widgets::kEspItemCount] = {};

    bool  g_espExpanded[2] = { false, false };
    float g_espExpandT[2] = { 0.0f, 0.0f };
    float g_espExpanderHover[2] = { 0.0f, 0.0f };

    bool  g_flagsExpanded[2] = { false, false };
    float g_flagsExpandT[2] = { 0.0f, 0.0f };
    float g_flagsExpanderHover[2] = { 0.0f, 0.0f };

    bool  g_offscreenExpanded[2] = { false, false };
    float g_offscreenExpandT[2] = { 0.0f, 0.0f };
    float g_offscreenExpanderHover[2] = { 0.0f, 0.0f };

    static float  g_linearAnim = 0.0f;
    static ImVec2 g_menuPos = ImVec2(-1.0f, -1.0f);
    static bool   g_dragging = false;
    static ImVec2 g_dragDelta = ImVec2(0.0f, 0.0f);

    static float  g_safeModeHover = 0.0f;
    static float  g_vsyncHover = 0.0f;
    static float  g_tearingHover = 0.0f;
    static float  g_fullscreenHover = 0.0f;
    static float  g_unloadHover = 0.0f;
    static bool   g_requestSafeModeConfirm = false;
    static bool   g_requestUnloadConfirm = false;

    static bool   s_aimKeyCapture = false;
    static bool   s_aimKeyWaitRelease = false;
    static bool   s_aimDefaultsApplied = false;

    static float  g_aimEnableHover = 0.0f;
    static float  g_aimEnableSel = 0.0f;
    static bool   g_aimModeExpanded = false;
    static float  g_aimModeExpandT = 0.0f;
    static float  g_aimModeHover = 0.0f;
    static bool   g_aimHitsExpanded = false;
    static float  g_aimHitsExpandT = 0.0f;
    static float  g_aimHitsHover = 0.0f;
    static float  g_aimModeOptHover[3] = { 0.0f, 0.0f, 0.0f };
    static float  g_aimModeOptSel[3] = { 0.0f, 0.0f, 0.0f };
    static float  g_aimHitboxHover[7] = {};
    static float  g_aimHitboxSel[7] = {};

    static constexpr float kMenuAnimDur = 0.10f;

    struct SubTabDef {
        const char* names[8];
        int count;
    };

    static const SubTabDef kSubTabs[5] = {
        { { "Aimbot", "Triggerbot", "Recoil", nullptr, nullptr, nullptr, nullptr, nullptr }, 3 },
        { { "Enemy", "Team", "Local", "World", nullptr, nullptr, nullptr, nullptr }, 4 },
        { { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }, 0 },
        { { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }, 0 },
        { { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }, 0 },
    };

    static const char* GetKeyName(int vk) {
        static char buf[32];
        if (vk <= 0) return "None";
        switch (vk) {
        case VK_LBUTTON:  return "M1";
        case VK_RBUTTON:  return "M2";
        case VK_MBUTTON:  return "M3";
        case VK_XBUTTON1: return "M4";
        case VK_XBUTTON2: return "M5";
        case VK_LSHIFT:   return "LShift";
        case VK_RSHIFT:   return "RShift";
        case VK_LCONTROL: return "LCtrl";
        case VK_RCONTROL: return "RCtrl";
        case VK_LMENU:    return "LAlt";
        case VK_RMENU:    return "RAlt";
        case VK_SPACE:    return "Space";
        case VK_TAB:      return "Tab";
        case VK_RETURN:   return "Enter";
        case VK_ESCAPE:   return "Esc";
        case VK_BACK:     return "Back";
        case VK_INSERT:   return "Ins";
        case VK_DELETE:   return "Del";
        case VK_HOME:     return "Home";
        case VK_END:      return "End";
        case VK_PRIOR:    return "PgUp";
        case VK_NEXT:     return "PgDn";
        case VK_CAPITAL:  return "Caps";
        case VK_NUMLOCK:  return "Num";
        case VK_SCROLL:   return "Scrl";
        }
        if (vk >= 'A' && vk <= 'Z') { buf[0] = (char)vk; buf[1] = 0; return buf; }
        if (vk >= '0' && vk <= '9') { buf[0] = (char)vk; buf[1] = 0; return buf; }
        if (vk >= VK_F1 && vk <= VK_F12) {
            int n = vk - VK_F1 + 1;
            if (n < 10) { buf[0] = 'F'; buf[1] = (char)('0' + n); buf[2] = 0; }
            else { buf[0] = 'F'; buf[1] = '1'; buf[2] = (char)('0' + (n - 10)); buf[3] = 0; }
            return buf;
        }
        if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
            buf[0] = 'N'; buf[1] = (char)('0' + (vk - VK_NUMPAD0)); buf[2] = 0; return buf;
        }
        snprintf(buf, sizeof(buf), "VK%02X", vk);
        return buf;
    }

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
            case 24: return config.esp_enemy_offscreen_color;
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
            case 24: return config.esp_team_offscreen_color;
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
            if (configBrowser.selected != clicked) Framework::PlayClickSound();
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

        ImFont* iconFont = Framework::GetIconFont();

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
            Framework::PlayClickSound();

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

    static void DrawChevronLocal(ImDrawList* dl, ImVec2 c, float size,
        float angle, ImU32 col, float th)
    {
        const float h = size * 0.5f;
        const float ca = cosf(angle);
        const float sa = sinf(angle);

        auto rot = [&](float x, float y) -> ImVec2 {
            return ImVec2(c.x + x * ca - y * sa, c.y + x * sa + y * ca);
            };

        const ImVec2 a = rot(-h * 0.40f, -h);
        const ImVec2 b = rot(h * 0.40f, 0.0f);
        const ImVec2 d = rot(-h * 0.40f, h);

        dl->AddLine(a, b, col, th);
        dl->AddLine(b, d, col, th);
    }

    static void DrawAimbotCard(ImDrawList* dl, ImVec2 min, ImVec2 max,
        float alpha, float s, ImFont* textFont, float labelFs, float dt)
    {
        const bool wasCapturing = s_aimKeyCapture;

        if (s_aimKeyCapture) {
            if (s_aimKeyWaitRelease) {
                if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
                    s_aimKeyWaitRelease = false;
                }
            }
            else {
                for (int vk = 1; vk < 256; ++vk) {
                    if (GetAsyncKeyState(vk) & 0x8000) {
                        config.aim_key = (vk == VK_ESCAPE) ? 0 : vk;
                        s_aimKeyCapture = false;
                        break;
                    }
                }
            }
        }

        const int a8 = (int)(alpha * 255.0f);
        const int borderA = (int)(alpha * 70.0f);
        dl->AddRectFilled(min, max, IM_COL32(26, 26, 34, a8), 6.0f);
        dl->AddRect(min, max, IM_COL32(189, 176, 255, borderA), 6.0f, 0, 1.0f);

        const float pad = 14.0f * s;
        const float rowH = 24.0f * s;
        const float optRowH = 22.0f * s;
        const float cbSize = 16.0f * s;
        const float cbRadius = 4.0f * s;
        const float iconSz = 11.0f * s;
        const float expGap = 6.0f * s;
        const float indent = 16.0f * s;

        const float contentX = min.x + pad;
        const float contentW = (max.x - min.x) - pad * 2.0f;

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const float kApproach = 1.0f - expf(-18.0f * dt);

        auto drawCheckbox = [&](ImVec2 cbMin, bool on, float hover, float aMul) {
            const float a = alpha * aMul;
            const float sv = on ? 1.0f : 0.0f;
            const float st = (sv > hover) ? sv : hover;
            const int rC = (int)(120 + (189 - 120) * st);
            const int gC = (int)(110 + (176 - 110) * st);
            const int bC = (int)(180 + (255 - 180) * st);
            const int fillA = (int)(a * (0.35f + 0.65f * sv) * 255.0f);
            const int ringA = (int)(a * 0.55f * hover * 255.0f);
            const ImVec2 cbMax(cbMin.x + cbSize, cbMin.y + cbSize);
            dl->AddRectFilled(cbMin, cbMax, IM_COL32(rC, gC, bC, fillA), cbRadius);
            if (ringA > 2)
                dl->AddRect(cbMin, cbMax, IM_COL32(rC, gC, bC, ringA), cbRadius, 0, 1.5f * s);
            };

        float rowY = min.y + pad;

        {
            const float cy = rowY + rowH * 0.5f;
            const ImVec2 cbMin(max.x - pad - cbSize, cy - cbSize * 0.5f);
            const ImVec2 cbMax(cbMin.x + cbSize, cbMin.y + cbSize);

            const bool hov = mouse.x >= cbMin.x && mouse.x <= cbMax.x
                && mouse.y >= cbMin.y && mouse.y <= cbMax.y;

            if (hov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                config.aim_enable = !config.aim_enable;
            }

            g_aimEnableHover += ((hov ? 1.0f : 0.0f) - g_aimEnableHover) * kApproach;
            g_aimEnableSel += (((config.aim_enable) ? 1.0f : 0.0f) - g_aimEnableSel) * kApproach;

            drawCheckbox(cbMin, config.aim_enable, g_aimEnableHover, 1.0f);

            if (textFont) {
                const char* lbl = "Enable Aimbot";
                const ImVec2 lsz = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, lbl);
                const float ly = cy - lsz.y * 0.5f;
                dl->AddText(textFont, labelFs, ImVec2(floorf(contentX + 0.5f), floorf(ly + 0.5f)),
                    IM_COL32(200, 195, 235, (int)(alpha * 255.0f)), lbl);
            }

            rowY += rowH;
        }

        {
            const float cy = rowY + rowH * 0.5f;
            const char* lbl = "Mode";
            const ImVec2 lsz = textFont
                ? textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, lbl)
                : ImVec2(30.0f * s, labelFs);

            if (textFont)
                dl->AddText(textFont, labelFs,
                    ImVec2(floorf(contentX + 0.5f), floorf(cy - lsz.y * 0.5f + 0.5f)),
                    IM_COL32(200, 195, 235, (int)(alpha * 255.0f)), lbl);

            const ImVec2 expC(contentX + lsz.x + expGap + iconSz * 0.5f, cy);
            const float eh = iconSz * 0.75f;
            const ImVec2 expMin(expC.x - eh, expC.y - eh);
            const ImVec2 expMax(expC.x + eh, expC.y + eh);

            const bool expHov = mouse.x >= expMin.x && mouse.x <= expMax.x
                && mouse.y >= expMin.y && mouse.y <= expMax.y;

            if (expHov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                g_aimModeExpanded = !g_aimModeExpanded;
            }

            g_aimModeHover += ((expHov ? 1.0f : 0.0f) - g_aimModeHover) * kApproach;
            g_aimModeExpandT += (((g_aimModeExpanded) ? 1.0f : 0.0f) - g_aimModeExpandT) * kApproach;

            const int eR = (int)(150 + (189 - 150) * g_aimModeHover);
            const int eG = (int)(140 + (176 - 140) * g_aimModeHover);
            const int eB = (int)(205 + (255 - 205) * g_aimModeHover);
            const ImU32 eCol = IM_COL32(eR, eG, eB, (int)(alpha * 255.0f));
            DrawChevronLocal(dl, expC, iconSz, 1.5707963f * g_aimModeExpandT, eCol, 1.6f * s);

            static const char* modes[] = { "Toggle", "Hold", "Always" };
            const char* mTxt = modes[config.aim_mode];
            if (textFont) {
                const float mx = expMax.x + 6.0f * s;
                const ImVec2 msz = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, mTxt);
                const float my = cy - msz.y * 0.5f;
                dl->AddText(textFont, labelFs, ImVec2(floorf(mx + 0.5f), floorf(my + 0.5f)),
                    IM_COL32(200, 190, 240, (int)(alpha * 255.0f)), mTxt);
            }

            const float keyW = 54.0f * s;
            const float keyH = 18.0f * s;
            const ImVec2 kMin(max.x - pad - keyW, cy - keyH * 0.5f);
            const ImVec2 kMax(kMin.x + keyW, kMin.y + keyH);

            ImGui::SetCursorScreenPos(kMin);
            ImGui::InvisibleButton("##aim_key_btn", ImVec2(keyW, keyH));
            const bool kHov = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked(0) && !wasCapturing) {
                s_aimKeyCapture = true;
                s_aimKeyWaitRelease = true;
            }

            dl->AddRectFilled(kMin, kMax, IM_COL32(30, 30, 40, (int)(alpha * 255)), 3.0f * s);
            dl->AddRect(kMin, kMax, IM_COL32(120, 110, 180, (int)(alpha * (kHov ? 0.8f : 0.4f) * 255)),
                3.0f * s, 0, 1.0f * s);
            if (s_aimKeyCapture)
                dl->AddRectFilled(kMin, kMax, IM_COL32(189, 176, 255, (int)(alpha * 0.2f * 255)), 3.0f * s);

            if (textFont) {
                const char* kTxt = s_aimKeyCapture ? "..." : GetKeyName(config.aim_key);
                const ImVec2 kSz = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, kTxt);
                const float ktx = kMin.x + (keyW - kSz.x) * 0.5f;
                const float kty = kMin.y + (keyH - kSz.y) * 0.5f;
                dl->AddText(textFont, labelFs, ImVec2(ktx, kty),
                    IM_COL32(200, 195, 235, (int)(alpha * 255.0f)), kTxt);
            }

            rowY += rowH;
        }

        if (g_aimModeExpandT > 0.001f) {
            static const char* modes[] = { "Toggle", "Hold", "Always" };
            const float rowA = alpha * g_aimModeExpandT;

            for (int i = 0; i < 3; ++i) {
                const ImVec2 rMin(contentX + indent, rowY);
                const ImVec2 rMax(max.x - pad, rowY + optRowH);

                ImGui::SetCursorScreenPos(rMin);
                ImGui::PushID(1000 + i);
                ImGui::InvisibleButton("##opt", ImVec2(rMax.x - rMin.x, optRowH));
                ImGui::PopID();

                const bool hov = ImGui::IsItemHovered();
                const bool sel = (config.aim_mode == i);

                g_aimModeOptHover[i] += ((hov ? 1.0f : 0.0f) - g_aimModeOptHover[i]) * kApproach;
                g_aimModeOptSel[i] += (((sel) ? 1.0f : 0.0f) - g_aimModeOptSel[i]) * kApproach;

                if (ImGui::IsItemClicked(0)) {
                    Framework::PlayClickSound();
                    config.aim_mode = i;
                    g_aimModeExpanded = false;
                }

                const float st = (g_aimModeOptSel[i] > g_aimModeOptHover[i]) ? g_aimModeOptSel[i] : g_aimModeOptHover[i];

                if (st > 0.005f) {
                    dl->AddRectFilled(rMin, rMax,
                        IM_COL32(120, 110, 180, (int)(rowA * 0.22f * st * 255.0f)), 3.0f * s);
                }

                if (textFont) {
                    const char* txt = modes[i];
                    const ImVec2 tsz = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, txt);
                    const float tx = rMin.x + 8.0f * s;
                    const float ty = rMin.y + (optRowH - tsz.y) * 0.5f;

                    const int rC = (int)(150 + (235 - 150) * st);
                    const int gC = (int)(140 + (228 - 140) * st);
                    const int bC = (int)(205 + (255 - 205) * st);

                    dl->AddText(textFont, labelFs, ImVec2(floorf(tx + 0.5f), floorf(ty + 0.5f)),
                        IM_COL32(rC, gC, bC, (int)(rowA * 255.0f)), txt);
                }

                if (g_aimModeOptSel[i] > 0.01f) {
                    const float uH = 2.0f * s;
                    const float uW = (rMax.x - rMin.x) * 0.4f * g_aimModeOptSel[i];
                    const float uY = rMax.y - uH - 1.0f * s;
                    const float cx = (rMin.x + rMax.x) * 0.5f;
                    dl->AddRectFilled(
                        ImVec2(cx - uW * 0.5f, uY),
                        ImVec2(cx + uW * 0.5f, uY + uH),
                        IM_COL32(189, 176, 255, (int)(g_aimModeOptSel[i] * rowA * 255.0f)),
                        uH * 0.5f);
                }

                rowY += optRowH * g_aimModeExpandT;
            }
        }

        {
            const float cy = rowY + rowH * 0.5f;
            const char* lbl = "FOV";
            const ImVec2 lsz = textFont
                ? textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, lbl)
                : ImVec2(24.0f * s, labelFs);

            if (textFont)
                dl->AddText(textFont, labelFs,
                    ImVec2(floorf(contentX + 0.5f), floorf(cy - lsz.y * 0.5f + 0.5f)),
                    IM_COL32(200, 195, 235, (int)(alpha * 255.0f)), lbl);

            const float valueW = 28.0f * s;
            const float sliderX = contentX + lsz.x + 8.0f * s;
            const float sliderW = (contentX + contentW) - sliderX - valueW - 4.0f * s;
            const float sliderY = cy - 2.0f * s;

            if (sliderW > 16.0f * s) {
                Widgets::Rect sr = {
                    ImVec2(sliderX, sliderY),
                    ImVec2(sliderX + sliderW, sliderY + 4.0f * s) };
                Widgets::DrawDashedSlider(dl, sr, &config.aim_fov, 0.0f, 180.0f,
                    alpha, s, "##aim_fov_s");
            }

            char buf[16];
            snprintf(buf, sizeof(buf), "%.0f", config.aim_fov);
            if (textFont) {
                const float tw = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, buf).x;
                dl->AddText(textFont, labelFs,
                    ImVec2(contentX + contentW - tw, cy - labelFs * 0.5f),
                    IM_COL32(200, 190, 240, (int)(alpha * 255.0f)), buf);
            }

            rowY += rowH;
        }

        {
            const float cy = rowY + rowH * 0.5f;
            const char* lbl = "Hitboxes";
            const ImVec2 lsz = textFont
                ? textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, lbl)
                : ImVec2(24.0f * s, labelFs);

            if (textFont)
                dl->AddText(textFont, labelFs,
                    ImVec2(floorf(contentX + 0.5f), floorf(cy - lsz.y * 0.5f + 0.5f)),
                    IM_COL32(200, 195, 235, (int)(alpha * 255.0f)), lbl);

            const ImVec2 expC(contentX + lsz.x + expGap + iconSz * 0.5f, cy);
            const float eh = iconSz * 0.75f;
            const ImVec2 expMin(expC.x - eh, expC.y - eh);
            const ImVec2 expMax(expC.x + eh, expC.y + eh);

            const bool expHov = mouse.x >= expMin.x && mouse.x <= expMax.x
                && mouse.y >= expMin.y && mouse.y <= expMax.y;

            if (expHov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                g_aimHitsExpanded = !g_aimHitsExpanded;
            }

            g_aimHitsHover += ((expHov ? 1.0f : 0.0f) - g_aimHitsHover) * kApproach;
            g_aimHitsExpandT += (((g_aimHitsExpanded) ? 1.0f : 0.0f) - g_aimHitsExpandT) * kApproach;

            const int eR = (int)(150 + (189 - 150) * g_aimHitsHover);
            const int eG = (int)(140 + (176 - 140) * g_aimHitsHover);
            const int eB = (int)(205 + (255 - 205) * g_aimHitsHover);
            const ImU32 eCol = IM_COL32(eR, eG, eB, (int)(alpha * 255.0f));
            DrawChevronLocal(dl, expC, iconSz, 1.5707963f * g_aimHitsExpandT, eCol, 1.6f * s);

            static const char* kHitboxNames[] = {
                "Head", "Neck", "Chest", "Stomach", "Arms", "Legs", "Feet"
            };
            char hitboxLabel[64] = {};
            int selCount = 0;
            int firstSel = -1;
            for (int i = 0; i < 7; ++i) {
                if (config.aim_hitbox & (1 << i)) {
                    if (firstSel < 0) firstSel = i;
                    selCount++;
                }
            }
            if (selCount == 0)
                snprintf(hitboxLabel, sizeof(hitboxLabel), "None");
            else if (selCount == 7)
                snprintf(hitboxLabel, sizeof(hitboxLabel), "All");
            else if (selCount == 1)
                snprintf(hitboxLabel, sizeof(hitboxLabel), "%s", kHitboxNames[firstSel]);
            else
                snprintf(hitboxLabel, sizeof(hitboxLabel), "%s +%d",
                    kHitboxNames[firstSel], selCount - 1);

            if (textFont) {
                const float mx = expMax.x + 6.0f * s;
                const ImVec2 msz = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, hitboxLabel);
                const float my = cy - msz.y * 0.5f;
                dl->AddText(textFont, labelFs, ImVec2(floorf(mx + 0.5f), floorf(my + 0.5f)),
                    IM_COL32(200, 190, 240, (int)(alpha * 255.0f)), hitboxLabel);
            }

            rowY += rowH;
        }

        if (g_aimHitsExpandT > 0.001f) {
            static const char* kHitboxNames[] = {
                "Head", "Neck", "Chest", "Stomach", "Arms", "Legs", "Feet"
            };
            const float rowA = alpha * g_aimHitsExpandT;

            for (int i = 0; i < 7; ++i) {
                const ImVec2 rMin(contentX + indent, rowY);
                const ImVec2 rMax(max.x - pad, rowY + optRowH);
                const ImVec2 cbMin(max.x - pad - cbSize, rMin.y + (optRowH - cbSize) * 0.5f);
                const ImVec2 cbMax(cbMin.x + cbSize, cbMin.y + cbSize);

                const bool on = (config.aim_hitbox >> i) & 1;

                ImGui::SetCursorScreenPos(cbMin);
                ImGui::PushID(2000 + i);
                ImGui::InvisibleButton("##hb", ImVec2(cbSize, cbSize));
                ImGui::PopID();

                const bool hov = ImGui::IsItemHovered();
                if (ImGui::IsItemClicked(0)) {
                    Framework::PlayClickSound();
                    if (on) config.aim_hitbox &= ~(1 << i);
                    else    config.aim_hitbox |= (1 << i);
                }

                g_aimHitboxHover[i] += ((hov ? 1.0f : 0.0f) - g_aimHitboxHover[i]) * kApproach;
                g_aimHitboxSel[i] += (((on) ? 1.0f : 0.0f) - g_aimHitboxSel[i]) * kApproach;

                drawCheckbox(cbMin, on, g_aimHitboxHover[i], g_aimHitsExpandT);

                if (textFont) {
                    const char* txt = kHitboxNames[i];
                    const ImVec2 tsz = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, txt);
                    const float tx = rMin.x;
                    const float ty = rMin.y + (optRowH - tsz.y) * 0.5f;

                    const float sv = on ? 1.0f : 0.0f;
                    const float st = (sv > g_aimHitboxHover[i]) ? sv : g_aimHitboxHover[i];
                    const int lr = (int)(190 + (235 - 190) * st);
                    const int lg = (int)(185 + (228 - 185) * st);
                    const int lb = (int)(215 + (255 - 215) * st);

                    dl->AddText(textFont, labelFs, ImVec2(floorf(tx + 0.5f), floorf(ty + 0.5f)),
                        IM_COL32(lr, lg, lb, (int)(rowA * 255.0f)), txt);
                }

                rowY += optRowH * g_aimHitsExpandT;
            }
        }

        {
            const float cy = rowY + rowH * 0.5f;
            const char* lbl = "Smooth";
            if (textFont) {
                const ImVec2 lsz = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, lbl);
                const float ly = cy - lsz.y * 0.5f;
                dl->AddText(textFont, labelFs, ImVec2(floorf(contentX + 0.5f), floorf(ly + 0.5f)),
                    IM_COL32(200, 195, 235, (int)(alpha * 255.0f)), lbl);
            }

            char buf[16];
            snprintf(buf, sizeof(buf), "%.0f", config.aim_smooth);
            if (textFont) {
                const float tw = textFont->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, buf).x;
                dl->AddText(textFont, labelFs,
                    ImVec2(contentX + contentW - tw, cy - labelFs * 0.5f),
                    IM_COL32(200, 190, 240, (int)(alpha * 255.0f)), buf);
            }

            rowY += rowH;
        }

        {
            const float sliderTop = rowY - 3.0f * s;
            Widgets::Rect sr = {
                ImVec2(contentX, sliderTop),
                ImVec2(contentX + contentW, sliderTop + 10.0f * s) };
            Widgets::DrawDashedSlider(dl, sr, &config.aim_smooth, 0.0f, 100.0f,
                alpha, s, "##aim_smooth_s");
            rowY += 13.0f * s;
        }
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
        const float rowH = 24.0f * s;
        const float cbSize = 16.0f * s;

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const bool interactive = (a > 0.99f);
        const float kApproach = 1.0f - expf(-18.0f * dt);

        const float cbX1 = cmax.x - cp;
        const float cbX0 = cbX1 - cbSize;
        const float labelX = cmin.x + cp;

        const float safeRowY = cmin.y + cp;
        const float vsyncRowY = safeRowY + rowH;
        const float tearRowY = vsyncRowY + rowH;
        const float fullscreenRowY = tearRowY + rowH;

        {
            const float cbY0 = safeRowY + (rowH - cbSize) * 0.5f;
            const float cbY1 = cbY0 + cbSize;
            const bool hov = interactive
                && mouse.x >= cbX0 && mouse.x <= cbX1
                && mouse.y >= cbY0 && mouse.y <= cbY1;

            g_safeModeHover += ((hov ? 1.0f : 0.0f) - g_safeModeHover) * kApproach;

            const bool enabled = config.safe_mode;
            const float selT = enabled ? 1.0f : 0.0f;
            const float st = (selT > g_safeModeHover) ? selT : g_safeModeHover;
            const int mR = (int)(120 + (189 - 120) * st);
            const int mG = (int)(110 + (176 - 110) * st);
            const int mB = (int)(180 + (255 - 180) * st);
            const int mFillA = (int)(cardAlpha * (0.35f + 0.65f * selT) * 255.0f);
            const int mRingA = (int)(cardAlpha * 0.55f * g_safeModeHover * 255.0f);

            dl->AddRectFilled(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
                IM_COL32(mR, mG, mB, mFillA), 4.0f * s);
            if (mRingA > 2)
                dl->AddRect(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
                    IM_COL32(mR, mG, mB, mRingA), 4.0f * s, 0, 1.5f * s);

            if (font) {
                const ImVec2 lsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, "Safemode");
                const float ly = safeRowY + (rowH - lsz.y) * 0.5f;
                const int lr = (int)(190 + (235 - 190) * st);
                const int lg = (int)(185 + (228 - 185) * st);
                const int lb = (int)(215 + (255 - 215) * st);
                dl->AddText(font, labelFs, ImVec2(floorf(labelX + 0.5f), floorf(ly + 0.5f)),
                    IM_COL32(lr, lg, lb, (int)(cardAlpha * 255.0f)), "Safemode");
            }

            if (hov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                if (enabled) {
                    if (config.safe_mode_no_warn)
                        config.safe_mode = false;
                    else
                        g_requestSafeModeConfirm = true;
                }
                else {
                    config.safe_mode = true;
                }
            }
        }

        {
            const float cbY0 = vsyncRowY + (rowH - cbSize) * 0.5f;
            const float cbY1 = cbY0 + cbSize;
            const bool hov = interactive
                && mouse.x >= cbX0 && mouse.x <= cbX1
                && mouse.y >= cbY0 && mouse.y <= cbY1;

            g_vsyncHover += ((hov ? 1.0f : 0.0f) - g_vsyncHover) * kApproach;

            const bool enabled = config.vsync;
            const float selT = enabled ? 1.0f : 0.0f;
            const float st = (selT > g_vsyncHover) ? selT : g_vsyncHover;
            const int mR = (int)(120 + (189 - 120) * st);
            const int mG = (int)(110 + (176 - 110) * st);
            const int mB = (int)(180 + (255 - 180) * st);
            const int mFillA = (int)(cardAlpha * (0.35f + 0.65f * selT) * 255.0f);
            const int mRingA = (int)(cardAlpha * 0.55f * g_vsyncHover * 255.0f);

            dl->AddRectFilled(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
                IM_COL32(mR, mG, mB, mFillA), 4.0f * s);
            if (mRingA > 2)
                dl->AddRect(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
                    IM_COL32(mR, mG, mB, mRingA), 4.0f * s, 0, 1.5f * s);

            if (font) {
                const ImVec2 lsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, "VSync");
                const float ly = vsyncRowY + (rowH - lsz.y) * 0.5f;
                const int lr = (int)(190 + (235 - 190) * st);
                const int lg = (int)(185 + (228 - 185) * st);
                const int lb = (int)(215 + (255 - 215) * st);
                dl->AddText(font, labelFs, ImVec2(floorf(labelX + 0.5f), floorf(ly + 0.5f)),
                    IM_COL32(lr, lg, lb, (int)(cardAlpha * 255.0f)), "VSync");
            }

            if (hov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                config.vsync = !config.vsync;
            }
        }

        {
            const float cbY0 = tearRowY + (rowH - cbSize) * 0.5f;
            const float cbY1 = cbY0 + cbSize;
            const bool hov = interactive
                && mouse.x >= cbX0 && mouse.x <= cbX1
                && mouse.y >= cbY0 && mouse.y <= cbY1;

            g_tearingHover += ((hov ? 1.0f : 0.0f) - g_tearingHover) * kApproach;

            const bool enabled = config.allow_tearing;
            const float selT = enabled ? 1.0f : 0.0f;
            const float st = (selT > g_tearingHover) ? selT : g_tearingHover;
            const int mR = (int)(120 + (189 - 120) * st);
            const int mG = (int)(110 + (176 - 110) * st);
            const int mB = (int)(180 + (255 - 180) * st);
            const int mFillA = (int)(cardAlpha * (0.35f + 0.65f * selT) * 255.0f);
            const int mRingA = (int)(cardAlpha * 0.55f * g_tearingHover * 255.0f);

            dl->AddRectFilled(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
                IM_COL32(mR, mG, mB, mFillA), 4.0f * s);
            if (mRingA > 2)
                dl->AddRect(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
                    IM_COL32(mR, mG, mB, mRingA), 4.0f * s, 0, 1.5f * s);

            if (font) {
                const ImVec2 lsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, "Allow Tearing");
                const float ly = tearRowY + (rowH - lsz.y) * 0.5f;
                const int lr = (int)(190 + (235 - 190) * st);
                const int lg = (int)(185 + (228 - 185) * st);
                const int lb = (int)(215 + (255 - 215) * st);
                dl->AddText(font, labelFs, ImVec2(floorf(labelX + 0.5f), floorf(ly + 0.5f)),
                    IM_COL32(lr, lg, lb, (int)(cardAlpha * 255.0f)), "Allow Tearing");
            }

            if (hov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                config.allow_tearing = !config.allow_tearing;
            }
        }

        {
            const float cbY0 = fullscreenRowY + (rowH - cbSize) * 0.5f;
            const float cbY1 = cbY0 + cbSize;
            const bool hov = interactive
                && mouse.x >= cbX0 && mouse.x <= cbX1
                && mouse.y >= cbY0 && mouse.y <= cbY1;

            g_fullscreenHover += ((hov ? 1.0f : 0.0f) - g_fullscreenHover) * kApproach;

            const bool enabled = config.fullscreen_mode;
            const float selT = enabled ? 1.0f : 0.0f;
            const float st = (selT > g_fullscreenHover) ? selT : g_fullscreenHover;
            const int mR = (int)(120 + (189 - 120) * st);
            const int mG = (int)(110 + (176 - 110) * st);
            const int mB = (int)(180 + (255 - 180) * st);
            const int mFillA = (int)(cardAlpha * (0.35f + 0.65f * selT) * 255.0f);
            const int mRingA = (int)(cardAlpha * 0.55f * g_fullscreenHover * 255.0f);

            dl->AddRectFilled(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
                IM_COL32(mR, mG, mB, mFillA), 4.0f * s);
            if (mRingA > 2)
                dl->AddRect(ImVec2(cbX0, cbY0), ImVec2(cbX1, cbY1),
                    IM_COL32(mR, mG, mB, mRingA), 4.0f * s, 0, 1.5f * s);

            if (font) {
                const ImVec2 lsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, "Fullscreen Mode");
                const float ly = fullscreenRowY + (rowH - lsz.y) * 0.5f;
                const int lr = (int)(190 + (235 - 190) * st);
                const int lg = (int)(185 + (228 - 185) * st);
                const int lb = (int)(215 + (255 - 215) * st);
                dl->AddText(font, labelFs, ImVec2(floorf(labelX + 0.5f), floorf(ly + 0.5f)),
                    IM_COL32(lr, lg, lb, (int)(cardAlpha * 255.0f)), "Fullscreen Mode");
            }

            if (hov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                config.fullscreen_mode = !config.fullscreen_mode;
                config.Save("config.ini");
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
            Framework::PlayClickSound();
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
                Framework::PlayClickSound();
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

    static void PlaceOffscreenSliders(ImDrawList* dl, ImVec2 drawMin, ImVec2 drawMax,
        float expandBox, float expandFlags, float expandOffscreen,
        float s, int curSub, float a, ImFont* font, float labelFs)
    {
        if (a < 0.5f) return;
        if (expandOffscreen < 0.01f) return;

        static const char* s_ids[2] = { "##offs_dist", "##offs_size" };

        for (int k = 0; k < 2; ++k) {
            const int idx = 25 + k;

            const float rowA = Widgets::RowAlpha(idx, expandBox, expandFlags, expandOffscreen);
            if (rowA < 0.01f) continue;

            float* valPtr = nullptr;
            float vmin = 0.0f;
            float vmax = 1.0f;
            char buf[32] = {};

            if (k == 0) {
                valPtr = (curSub == 0)
                    ? &config.esp_enemy_offscreen_distance
                    : &config.esp_team_offscreen_distance;
                vmin = 0.15f;
                vmax = 0.95f;
                snprintf(buf, sizeof(buf), "%.2f", *valPtr);
            }
            else {
                valPtr = (curSub == 0)
                    ? &config.esp_enemy_offscreen_size
                    : &config.esp_team_offscreen_size;
                vmin = 12.0f;
                vmax = 50.0f;
                snprintf(buf, sizeof(buf), "%.0f", *valPtr);
            }

            const float alpha = a * rowA;

            if (font) {
                const float labelCy = Widgets::RowCenterY(drawMin, idx, expandBox, expandFlags,
                    expandOffscreen, s);
                const ImVec2 tsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, buf);
                const float tx = drawMax.x - 14.0f * s - tsz.x;
                const float ty = labelCy - tsz.y * 0.5f;
                dl->AddText(font, labelFs,
                    ImVec2(floorf(tx + 0.5f), floorf(ty + 0.5f)),
                    IM_COL32(200, 190, 240, (int)(alpha * 255.0f)), buf);
            }

            Widgets::Rect sr = Widgets::GetSliderRect(
                drawMin, drawMax, idx, expandBox, expandFlags, expandOffscreen, s);

            if (sr.max.x - sr.min.x < 16.0f) continue;

            Widgets::DrawDashedSlider(dl, sr, valPtr, vmin, vmax, alpha, s, s_ids[k]);
        }
    }

    static void DrawHeader(ImDrawList* dl, ImVec2 pos, float menuW, float hdrH,
        float s, float a, float dt, bool& anyIconHover)
    {
        const ImGuiIO& io = ImGui::GetIO();
        const float time = (float)ImGui::GetTime();

        const ImU32 baseCol = IM_COL32(110, 100, 170, (int)(a * 110.0f));
        const ImU32 glowCol = IM_COL32(160, 140, 255, (int)(a * 255.0f));
        const ImU32 coreCol = IM_COL32(235, 228, 255, (int)(a * 255.0f));

        const float pulseWidth = menuW * 0.22f;
        const float travelSpan = menuW + pulseWidth * 2.0f;
        const float speed = menuW * 0.34f;
        const float cycle = fmodf(time * speed, travelSpan);
        const float u = cycle / travelSpan;
        const float eased = u * u * (3.0f - 2.0f * u);
        const float pulsePos = eased * travelSpan - pulseWidth;
        const float breathe = 0.88f + 0.12f * sinf(time * 1.4f);
        const float widthNow = pulseWidth * breathe;

        {
            const float lineY = pos.y + hdrH;
            Framework::DrawFlowLine(dl,
                ImVec2(pos.x, lineY),
                ImVec2(pos.x + menuW, lineY),
                baseCol, glowCol, coreCol,
                1.0f, pulsePos, widthNow);
        }

        ImFont* font = io.FontDefault ? io.FontDefault : ImGui::GetFont();
        const float textFs = 26.0f * s;
        if (textFs <= 5.0f) return;

        auto smooth = [](float t) {
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            return t * t * (3.0f - 2.0f * t);
            };

        const float tp = smooth((a - 0.15f) / 0.85f);
        const int ta = (int)(tp * 255.0f);

        const ImVec2 titleSize = font->CalcTextSizeA(textFs, FLT_MAX, 0.0f, "Oblivion");

        const float textX = pos.x + 12.0f * s;
        const float textY = pos.y + (hdrH - titleSize.y) * 0.5f;

        const float textOffsetOnLine = textX - pos.x;
        const float textCenterOnLine = textOffsetOnLine + titleSize.x * 0.5f;

        const float syncRange = pulseWidth * 1.6f;
        float prox = 1.0f - fabsf(pulsePos - textCenterOnLine) / syncRange;
        if (prox < 0.0f) prox = 0.0f;
        if (prox > 1.0f) prox = 1.0f;
        prox = prox * prox * (3.0f - 2.0f * prox);

        const int glowAlpha = (int)(ta * (0.45f + 0.55f * prox));

        constexpr float off = 1.0f;
        const ImU32 glowOuter = IM_COL32(140, 120, 255, glowAlpha / 3);
        const ImU32 glowInner = IM_COL32(180, 165, 255, glowAlpha / 2);
        const ImU32 coreText = IM_COL32(210, 200, 255, ta);

        dl->AddText(font, textFs, ImVec2(textX - off * s, textY), glowOuter, "Oblivion");
        dl->AddText(font, textFs, ImVec2(textX + off * s, textY), glowOuter, "Oblivion");
        dl->AddText(font, textFs, ImVec2(textX, textY - off * s), glowOuter, "Oblivion");
        dl->AddText(font, textFs, ImVec2(textX, textY + off * s), glowOuter, "Oblivion");

        dl->AddText(font, textFs, ImVec2(textX - 0.5f, textY), glowInner, "Oblivion");
        dl->AddText(font, textFs, ImVec2(textX + 0.5f, textY), glowInner, "Oblivion");

        dl->AddText(font, textFs, ImVec2(textX, textY), coreText, "Oblivion");

        const float sweepW = titleSize.x * 0.32f;
        const float sweepX = textX + (pulsePos - textOffsetOnLine);
        const ImU32 sweepCol = IM_COL32(245, 240, 255, (int)(ta * prox * 0.95f + ta * 0.05f));

        dl->PushClipRect(
            ImVec2(sweepX, textY - 1.0f),
            ImVec2(sweepX + sweepW, textY + titleSize.y + 1.0f),
            true);
        dl->AddText(font, textFs, ImVec2(textX, textY), sweepCol, "Oblivion");
        dl->PopClipRect();

        const float boxSz = 34.0f * s;
        const float iconGap = 8.0f * s;
        const float rightMargin = 14.0f * s;
        const float subFs = 14.0f * s;
        const float subPadX = 10.0f * s;
        const float subGap = 2.0f * s;

        float subWidths[5][8] = {};
        float subTotalW[5] = {};

        if (subFs > 4.0f) {
            for (int i = 0; i < 5; ++i) {
                if (kSubTabs[i].count == 0) continue;
                for (int k = 0; k < kSubTabs[i].count; ++k) {
                    ImVec2 sz = font->CalcTextSizeA(subFs, FLT_MAX, 0.0f, kSubTabs[i].names[k]);
                    subWidths[i][k] = sz.x + subPadX * 2.0f;
                    subTotalW[i] += subWidths[i][k];
                    if (k < kSubTabs[i].count - 1) subTotalW[i] += subGap;
                }
            }
        }

        auto animatedSubW = [&](int i) -> float {
            if (kSubTabs[i].count == 0) return 0.0f;
            return g_iconSel[i] * (subTotalW[i] + iconGap);
            };

        float totalInsertedW = 0.0f;
        for (int i = 0; i < 5; ++i) totalInsertedW += animatedSubW(i);

        const float baseTotalW = 5.0f * boxSz + 4.0f * iconGap;
        const float iconStartX = pos.x + menuW - rightMargin - baseTotalW - totalInsertedW;
        const float iconCY = pos.y + hdrH * 0.5f;

        auto iconOffset = [&](int i) -> float {
            float o = 0.0f;
            for (int j = 0; j < i; ++j) {
                o += boxSz + iconGap;
                o += animatedSubW(j);
            }
            return o;
            };

        auto subStartXFor = [&](int i) -> float {
            return iconStartX + iconOffset(i) + boxSz + iconGap;
            };

        const ImVec2 mouse = io.MousePos;
        int hoverIdx = -1;
        int hoverSub[5] = { -1, -1, -1, -1, -1 };

        for (int i = 0; i < 5; ++i) {
            if (kSubTabs[i].count == 0) continue;
            if (i != g_activeTab) continue;
            if (g_iconSel[i] < 0.35f) continue;

            float scanX = subStartXFor(i);
            const float by0 = iconCY - boxSz * 0.42f;
            const float by1 = iconCY + boxSz * 0.42f;

            for (int k = 0; k < kSubTabs[i].count; ++k) {
                const float bx0 = scanX;
                const float bx1 = scanX + subWidths[i][k];

                if (mouse.x >= bx0 && mouse.x <= bx1 &&
                    mouse.y >= by0 && mouse.y <= by1) {
                    hoverSub[i] = k;
                    anyIconHover = true;
                    if (ImGui::IsMouseClicked(0)) {
                        if (g_activeSub[i] != k) Framework::PlayClickSound();
                        g_activeSub[i] = k;
                    }
                }
                scanX += subWidths[i][k] + subGap;
            }
        }

        for (int i = 0; i < 5; ++i) {
            const float ix0 = iconStartX + iconOffset(i);
            const float ix1 = ix0 + boxSz;
            const float iy0 = iconCY - boxSz * 0.5f;
            const float iy1 = iconCY + boxSz * 0.5f;

            if (mouse.x >= ix0 && mouse.x <= ix1 &&
                mouse.y >= iy0 && mouse.y <= iy1) {
                hoverIdx = i;
                anyIconHover = true;
                if (ImGui::IsMouseClicked(0)) {
                    if (g_activeTab != i) Framework::PlayClickSound();
                    g_activeTab = i;
                }
            }
        }

        const float kApproach = 1.0f - expf(-16.0f * dt);
        for (int i = 0; i < 5; ++i) {
            const float th = (i == hoverIdx) ? 1.0f : 0.0f;
            const float ts = (i == g_activeTab) ? 1.0f : 0.0f;
            g_iconHover[i] += (th - g_iconHover[i]) * kApproach;
            g_iconSel[i] += (ts - g_iconSel[i]) * kApproach;
        }

        for (int i = 0; i < 5; ++i) {
            if (kSubTabs[i].count == 0) continue;
            for (int k = 0; k < kSubTabs[i].count; ++k) {
                const float th = (k == hoverSub[i]) ? 1.0f : 0.0f;
                const float ts = (k == g_activeSub[i] && i == g_activeTab) ? 1.0f : 0.0f;
                g_subHover[i][k] += (th - g_subHover[i][k]) * kApproach;
                g_subSel[i][k] += (ts - g_subSel[i][k]) * kApproach;
            }
        }

        const float glyphFs = boxSz * 0.58f;

        for (int i = 0; i < 5; ++i) {
            const float ix0 = iconStartX + iconOffset(i);
            const float ix1 = ix0 + boxSz;
            const float cx = ix0 + boxSz * 0.5f;
            const float iy0 = iconCY - boxSz * 0.5f;
            const float iy1 = iconCY + boxSz * 0.5f;

            const float state = (g_iconSel[i] > g_iconHover[i]) ? g_iconSel[i] : g_iconHover[i];

            const float bgAlpha = g_iconSel[i] * 0.20f + g_iconHover[i] * 0.10f;
            if (bgAlpha > 0.005f) {
                dl->AddRectFilled(
                    ImVec2(ix0, iy0), ImVec2(ix1, iy1),
                    IM_COL32(189, 176, 255, (int)(bgAlpha * ta)),
                    boxSz * 0.22f);
            }

            const float iconOffsetOnLine = cx - pos.x;
            float iprox = 1.0f - fabsf(pulsePos - iconOffsetOnLine) / (pulseWidth * 1.3f);
            if (iprox < 0.0f) iprox = 0.0f;
            if (iprox > 1.0f) iprox = 1.0f;
            iprox = iprox * iprox * (3.0f - 2.0f * iprox);

            const float brightness = 0.55f + 0.45f * state + 0.15f * iprox;
            float alphaF = (ta / 255.0f) * brightness;
            if (alphaF > 1.0f) alphaF = 1.0f;

            const int rC = (int)(150 + (189 - 150) * state);
            const int gC = (int)(140 + (176 - 140) * state);
            const int bC = (int)(205 + (255 - 205) * state);

            const ImU32 iconCol = IM_COL32(rC, gC, bC, (int)(alphaF * 255.0f));

            Framework::DrawIconGlyph(dl, i, ImVec2(cx, iconCY), boxSz, glyphFs, iconCol);

            if (g_iconSel[i] > 0.01f) {
                const float uH = 2.0f * s;
                const float uW = boxSz * 0.55f * g_iconSel[i];
                const float uY = iy1 - uH - 1.0f * s;
                dl->AddRectFilled(
                    ImVec2(cx - uW * 0.5f, uY),
                    ImVec2(cx + uW * 0.5f, uY + uH),
                    IM_COL32(189, 176, 255, (int)(g_iconSel[i] * ta)),
                    uH * 0.5f);
            }
        }

        if (subFs > 4.0f) {
            for (int i = 0; i < 5; ++i) {
                if (kSubTabs[i].count == 0) continue;

                const float visualSelT = g_iconSel[i];
                if (visualSelT <= 0.01f) continue;

                float drawSubX = subStartXFor(i);
                const float sBy0 = iconCY - boxSz * 0.42f;
                const float sBy1 = iconCY + boxSz * 0.42f;

                for (int k = 0; k < kSubTabs[i].count; ++k) {
                    const float bx0 = drawSubX;
                    const float bx1 = drawSubX + subWidths[i][k];
                    const float cx = (bx0 + bx1) * 0.5f;
                    const float cy = (sBy0 + sBy1) * 0.5f;

                    const float st = (g_subSel[i][k] > g_subHover[i][k]) ? g_subSel[i][k] : g_subHover[i][k];
                    const float bgA = (g_subSel[i][k] * 0.18f + g_subHover[i][k] * 0.08f) * visualSelT;

                    if (bgA > 0.005f) {
                        dl->AddRectFilled(
                            ImVec2(bx0, sBy0), ImVec2(bx1, sBy1),
                            IM_COL32(189, 176, 255, (int)(bgA * ta)),
                            (sBy1 - sBy0) * 0.24f);
                    }

                    const char* label = kSubTabs[i].names[k];
                    const ImVec2 lsz = font->CalcTextSizeA(subFs, FLT_MAX, 0.0f, label);
                    const float tx = floorf(cx - lsz.x * 0.5f + 0.5f);
                    const float ty = floorf(cy - lsz.y * 0.5f + 0.5f);

                    const int rC = (int)(150 + (235 - 150) * st);
                    const int gC = (int)(140 + (228 - 140) * st);
                    const int bC = (int)(205 + (255 - 205) * st);
                    const int aC = (int)(ta * visualSelT * (0.70f + 0.30f * st));

                    const ImU32 txtCol = IM_COL32(rC, gC, bC, aC);
                    dl->AddText(font, subFs, ImVec2(tx, ty), txtCol, label);

                    if (g_subSel[i][k] > 0.01f) {
                        const float uH = 2.0f * s;
                        const float uW = subWidths[i][k] * 0.55f * g_subSel[i][k] * visualSelT;
                        const float uY = sBy1 + 1.0f * s;
                        dl->AddRectFilled(
                            ImVec2(cx - uW * 0.5f, uY),
                            ImVec2(cx + uW * 0.5f, uY + uH),
                            IM_COL32(189, 176, 255, (int)(g_subSel[i][k] * ta * visualSelT)),
                            uH * 0.5f);
                    }

                    drawSubX += subWidths[i][k] + subGap;
                }
            }
        }
    }

    static void DrawStars(ImDrawList* dl, ImVec2 min, ImVec2 max, float time, float alpha)
    {
        if (alpha < 0.02f) return;
        if (max.x <= min.x || max.y <= min.y) return;

        const float w = max.x - min.x;
        const float h = max.y - min.y;

        const int kLayers = 5;
        const int kPerLayer[5] = { 520, 340, 210, 110, 48 };

        const float layerAlpha[5] = { 0.32f, 0.55f, 0.78f, 0.92f, 1.00f };
        const float sizeMin[5] = { 0.18f, 0.28f, 0.44f, 0.64f, 0.88f };
        const float sizeMax[5] = { 0.40f, 0.58f, 0.85f, 1.15f, 1.55f };
        const float speedMin[5] = { 0.20f, 0.34f, 0.52f, 0.72f, 0.95f };
        const float speedMax[5] = { 0.75f, 1.05f, 1.45f, 1.90f, 2.40f };

        for (int layer = 0; layer < kLayers; ++layer) {
            const int n = kPerLayer[layer];
            for (int i = 0; i < n; ++i) {
                const unsigned int seed =
                    (unsigned int)(layer * 2371 + i * 53 + 11);

                const float rx = Framework::Hash01(seed * 3u + 1u);
                const float ry = Framework::Hash01(seed * 5u + 7u);
                const float rp = Framework::Hash01(seed * 7u + 11u);
                const float rs = Framework::Hash01(seed * 11u + 13u);
                const float rv = Framework::Hash01(seed * 13u + 17u);
                const float rc = Framework::Hash01(seed * 17u + 19u);
                const float rsq = Framework::Hash01(seed * 19u + 23u);

                const float px = min.x + rx * w;
                const float py = min.y + ry * h;

                const float base = sizeMin[layer] + rs * (sizeMax[layer] - sizeMin[layer]);
                const float size = base * (0.80f + rsq * 0.40f);
                const float spd = speedMin[layer] + rv * (speedMax[layer] - speedMin[layer]);

                float tw = 0.5f + 0.5f * sinf(time * spd + rp * 6.2831853f);
                tw = tw * tw * tw;
                tw = tw * tw;

                const float starA = alpha * layerAlpha[layer] * (0.08f + 0.92f * tw);
                int a8 = (int)(starA * 255.0f);
                if (a8 < 3) continue;
                if (a8 > 255) a8 = 255;

                int rTint = 255;
                int gTint = 255;
                int bTint = 255;

                if (rc < 0.20f) {
                    rTint = 205; gTint = 218; bTint = 255;
                }
                else if (rc < 0.34f) {
                    rTint = 255; gTint = 240; bTint = 215;
                }
                else if (rc < 0.45f) {
                    rTint = 235; gTint = 225; bTint = 255;
                }

                const ImVec2 p(px, py);

                if (layer >= 3) {
                    const int haloA = (int)(a8 * 0.14f);
                    if (haloA > 2) {
                        dl->AddCircleFilled(p, size * 3.2f,
                            IM_COL32(rTint, gTint, bTint, haloA), 12);
                    }

                    const int midA = (int)(a8 * 0.38f);
                    if (midA > 2) {
                        dl->AddCircleFilled(p, size * 1.65f,
                            IM_COL32(rTint, gTint, bTint, midA), 12);
                    }
                }
                else if (layer == 2) {
                    const int midA = (int)(a8 * 0.28f);
                    if (midA > 2) {
                        dl->AddCircleFilled(p, size * 1.55f,
                            IM_COL32(rTint, gTint, bTint, midA), 10);
                    }
                }

                dl->AddCircleFilled(p, size,
                    IM_COL32(rTint, gTint, bTint, a8), 12);

                if (layer == 4 && a8 > 140) {
                    const int spikeA = (int)(a8 * 0.45f);
                    const float sLen = size * 3.0f;
                    const float sTh = 0.75f;
                    const ImU32 sc = IM_COL32(rTint, gTint, bTint, spikeA);

                    dl->AddLine(ImVec2(px - sLen, py), ImVec2(px + sLen, py), sc, sTh);
                    dl->AddLine(ImVec2(px, py - sLen), ImVec2(px, py + sLen), sc, sTh);
                }
            }
        }
    }

    void Render(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height) {
        (void)device; (void)context; (void)width; (void)height;

        if (!s_aimDefaultsApplied) {
            s_aimDefaultsApplied = true;
            if (config.aim_key == 0 && config.aim_mode == 0) {
                config.aim_key = VK_LBUTTON;
                config.aim_mode = 1;
            }
        }

        g_AllowMemoryWrite.store(!config.safe_mode, std::memory_order_relaxed);

        const ImGuiIO& io = ImGui::GetIO();

        float dt = io.DeltaTime;
        if (dt > 0.05f) dt = 0.05f;
        if (dt < 0.0f)  dt = 0.0f;
        if (std::isnan(dt) || std::isinf(dt)) dt = 0.016f;

        const bool open = config.show_menu;

        const float step = (kMenuAnimDur > 0.0f) ? (dt / kMenuAnimDur) : 1.0f;
        if (open) g_linearAnim = (std::min)(1.0f, g_linearAnim + step);
        else      g_linearAnim = (std::max)(0.0f, g_linearAnim - step);

        if (g_linearAnim < 0.001f && !open) {
            g_dragging = false;
            return;
        }

        const float raw = g_linearAnim;
        float a = raw * raw * (3.0f - 2.0f * raw);
        if (std::isnan(a) || std::isinf(a)) a = 1.0f;

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
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoBackground;

        ImGui::Begin("##oblivion_menu", nullptr, flags);

        const int a255 = (int)(a * 255.0f);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 pos = ImGui::GetWindowPos();

        const float hdrH = kHeaderH * s;

        dl->AddRectFilled(pos, ImVec2(pos.x + menuW, pos.y + menuH),
            IM_COL32(19, 19, 23, a255), 6.0f, ImDrawFlags_RoundCornersAll);

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

            DrawStars(dl,
                ImVec2(pos.x, starAreaY),
                ImVec2(pos.x + menuW, starAreaY + starAreaH),
                time, a * s);

            dl->PopClipRect();
        }

        bool anyIconHover = false;
        DrawHeader(dl, pos, menuW, hdrH, s, a, dt, anyIconHover);

        const bool showAimbotCard = (a > 0.5f)
            && g_activeTab == 0
            && g_activeSub[0] == 0;

        const bool showEspCard = (a > 0.5f)
            && g_activeTab == 1
            && (g_activeSub[1] == 0 || g_activeSub[1] == 1);

        const bool showConfigCards = (a > 0.5f) && g_activeTab == 3;
        const bool showSystemCard = (a > 0.5f) && g_activeTab == 4;

        ImFont* font = io.FontDefault ? io.FontDefault : ImGui::GetFont();
        const float labelFs = 15.0f * s;

        if (showAimbotCard) {
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

            DrawAimbotCard(dl, cmin, cmax, a * s, s, font, labelFs, dt);
        }
        else if (showEspCard) {
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

            const int curSub = g_activeSub[1];

            bool* vals[Widgets::kEspItemCount];
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
            vals[24] = (curSub == 0) ? &config.esp_enemy_offscreen : &config.esp_team_offscreen;
            vals[25] = nullptr;
            vals[26] = nullptr;

            const ImVec2 mouse = io.MousePos;
            const float kApproach = 1.0f - expf(-18.0f * dt);

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

            const float contentHeight = Widgets::ContentHeight(
                g_espExpandT[curSub],
                g_flagsExpandT[curSub],
                g_offscreenExpandT[curSub]) * s;
            ImGui::Dummy(ImVec2(cardW, contentHeight));

            const float expandBox = g_espExpandT[curSub];
            const float expandFlags = g_flagsExpandT[curSub];
            const float expandOffscreen = g_offscreenExpandT[curSub];

            const float scrollY = ImGui::GetScrollY();
            const ImVec2 drawMin(c0min.x, c0min.y - scrollY);
            const ImVec2 drawMax(c0max.x, c0max.y - scrollY);

            ImDrawList* cdl = ImGui::GetWindowDrawList();

            PlaceOffscreenSliders(cdl, drawMin, drawMax, expandBox, expandFlags,
                expandOffscreen, s, curSub, a, font, labelFs);

            cdl->PushClipRect(c0min, c0max, true);

            const bool mouseInCard = (a > 0.99f)
                && mouse.x >= c0min.x && mouse.x <= c0max.x
                && mouse.y >= c0min.y && mouse.y <= c0max.y;

            for (int i = 0; i < Widgets::kEspItemCount; ++i) {
                if (Widgets::IsSliderRow(i)) continue;
                const float rowA = Widgets::RowAlpha(i, expandBox, expandFlags, expandOffscreen);
                const bool interactive = mouseInCard && (rowA > 0.5f);

                Widgets::Rect cb = Widgets::GetCheckboxRect(
                    drawMin, drawMax, i, expandBox, expandFlags, expandOffscreen, s);

                const bool hov = interactive
                    && mouse.x >= cb.min.x && mouse.x <= cb.max.x
                    && mouse.y >= cb.min.y && mouse.y <= cb.max.y;

                if (hov && ImGui::IsMouseClicked(0)) {
                    Framework::PlayClickSound();
                    *(vals[i]) = !(*(vals[i]));
                }

                g_espHover[curSub][i] +=
                    ((hov ? 1.0f : 0.0f) - g_espHover[curSub][i]) * kApproach;
                g_espSel[curSub][i] +=
                    (((*(vals[i])) ? 1.0f : 0.0f) - g_espSel[curSub][i]) * kApproach;
            }

            static char s_popupId[Widgets::kEspItemCount][32] = {};

            for (int i = 0; i < Widgets::kEspItemCount; ++i) {
                if (Widgets::IsSliderRow(i)) continue;
                const float rowA = Widgets::RowAlpha(i, expandBox, expandFlags, expandOffscreen);
                const bool interactive = mouseInCard && (rowA > 0.5f);

                bool colorHov = false;

                float* colorPtr = Widgets::HasColor(i) ? GetEspColorPtr(curSub, i) : nullptr;

                if (colorPtr) {
                    Widgets::Rect cr = Widgets::GetColorRect(
                        drawMin, drawMax, i, expandBox, expandFlags, expandOffscreen, s);

                    colorHov = interactive
                        && mouse.x >= cr.min.x && mouse.x <= cr.max.x
                        && mouse.y >= cr.min.y && mouse.y <= cr.max.y;

                    if (!s_popupId[i][0]) {
                        snprintf(s_popupId[i], sizeof(s_popupId[i]),
                            "##esp_color_%d", i);
                    }

                    if (colorHov && ImGui::IsMouseClicked(0)) {
                        Framework::PlayClickSound();
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

                g_espColorHover[curSub][i] +=
                    ((colorHov ? 1.0f : 0.0f) - g_espColorHover[curSub][i]) * kApproach;
            }

            Widgets::Rect ex = Widgets::GetExpanderRect(drawMin, drawMax, s,
                expandBox, expandFlags, expandOffscreen, font, labelFs);
            const bool exHov = mouseInCard
                && mouse.x >= ex.min.x && mouse.x <= ex.max.x
                && mouse.y >= ex.min.y && mouse.y <= ex.max.y;

            if (exHov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                g_espExpanded[curSub] = !g_espExpanded[curSub];
            }

            g_espExpanderHover[curSub] +=
                ((exHov ? 1.0f : 0.0f) - g_espExpanderHover[curSub]) * kApproach;

            const float targetBox = g_espExpanded[curSub] ? 1.0f : 0.0f;
            g_espExpandT[curSub] +=
                (targetBox - g_espExpandT[curSub]) * kApproach;

            Widgets::Rect fex = Widgets::GetFlagsExpanderRect(drawMin, drawMax, s,
                expandBox, expandFlags, expandOffscreen, font, labelFs);
            const bool fexHov = mouseInCard
                && mouse.x >= fex.min.x && mouse.x <= fex.max.x
                && mouse.y >= fex.min.y && mouse.y <= fex.max.y;

            if (fexHov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                g_flagsExpanded[curSub] = !g_flagsExpanded[curSub];
            }

            g_flagsExpanderHover[curSub] +=
                ((fexHov ? 1.0f : 0.0f) - g_flagsExpanderHover[curSub]) * kApproach;

            const float targetFlags = g_flagsExpanded[curSub] ? 1.0f : 0.0f;
            g_flagsExpandT[curSub] +=
                (targetFlags - g_flagsExpandT[curSub]) * kApproach;

            Widgets::Rect oex = Widgets::GetOffscreenExpanderRect(drawMin, drawMax, s,
                expandBox, expandFlags, expandOffscreen, font, labelFs);
            const bool oexHov = mouseInCard
                && mouse.x >= oex.min.x && mouse.x <= oex.max.x
                && mouse.y >= oex.min.y && mouse.y <= oex.max.y;

            if (oexHov && ImGui::IsMouseClicked(0)) {
                Framework::PlayClickSound();
                g_offscreenExpanded[curSub] = !g_offscreenExpanded[curSub];
            }

            g_offscreenExpanderHover[curSub] +=
                ((oexHov ? 1.0f : 0.0f) - g_offscreenExpanderHover[curSub]) * kApproach;

            const float targetOffscreen = g_offscreenExpanded[curSub] ? 1.0f : 0.0f;
            g_offscreenExpandT[curSub] +=
                (targetOffscreen - g_offscreenExpandT[curSub]) * kApproach;

            const float* colorPtrs[Widgets::kEspItemCount] = {};
            for (int i = 0; i < Widgets::kEspItemCount; ++i) {
                if (Widgets::HasColor(i))
                    colorPtrs[i] = GetEspColorPtr(curSub, i);
            }

            Widgets::DrawEspCards(cdl, drawMin, drawMax, contentAlpha,
                g_espHover[curSub],
                g_espSel[curSub],
                g_espColorHover[curSub],
                colorPtrs,
                g_espExpandT[curSub],
                g_espExpanderHover[curSub],
                g_flagsExpandT[curSub],
                g_flagsExpanderHover[curSub],
                g_offscreenExpandT[curSub],
                g_offscreenExpanderHover[curSub],
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
        ImGui::PopStyleColor(1);
    }

    bool IsOpen() { return config.show_menu; }
    void Toggle() {}
}