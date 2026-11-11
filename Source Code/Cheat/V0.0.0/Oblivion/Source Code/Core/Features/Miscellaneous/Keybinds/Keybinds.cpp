#include "Keybinds.hpp"
#include "../../../GUI/Framework/Framework.hpp"
#include "../../../../Utilities/Config/Config.hpp"

#include <imgui.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

namespace Keybinds {

    static bool s_visible = true;

    bool Visible() { return s_visible; }
    void SetVisible(bool v) { s_visible = v; }
    void Toggle() { s_visible = !s_visible; }

    struct ActiveBindRow {
        std::string label;
        std::string keyName;
        int   category;
        bool  hasValue;
        float value;
        float vmin;
        float vmax;
        int   slot;
        bool  isOn;
    };

    static std::string CleanLabel(const std::string& label) {
        std::string r = label;
        static const char* suffixes[] = { " Enabled", " Enable" };
        for (const char* suf : suffixes) {
            size_t n = strlen(suf);
            if (r.size() > n && r.compare(r.size() - n, n, suf) == 0) {
                r.resize(r.size() - n);
                break;
            }
        }
        static const char* prefixes[] = { "Enabled ", "Enable " };
        for (const char* pre : prefixes) {
            size_t n = strlen(pre);
            if (r.size() > n && r.compare(0, n, pre) == 0) {
                r = r.substr(n);
                break;
            }
        }
        return r.empty() ? label : r;
    }

    static void CollectRows(std::vector<ActiveBindRow>& out) {
        auto addBool = [&](const Bind& b, int cat) {
            if (!b.enabled || b.key == 0) return;
            out.push_back({ b.label, GetKeyName(b.key), cat,
                false, 0.0f, 0.0f, 1.0f, b.slot, b.state });
            };
        auto addValue = [&](const ValueBind& vb, int cat, float vmin, float vmax) {
            if (!vb.enabled || vb.key == 0) return;
            out.push_back({ vb.label, GetKeyName(vb.key), cat,
                true, vb.value, vmin, vmax, vb.slot, false });
            };

        AimSettings* aims[64];
        int nAim = 0;
        aims[nAim++] = &config.aim_shared;
        for (auto& g : config.aim_groups)
            if (nAim < 64) aims[nAim++] = &g.settings;

        for (int i = 0; i < nAim; ++i) {
            AimSettings& a = *aims[i];
            addBool(a.bind_enable, 0);
            addBool(a.bind_visible_check, 0);
            addBool(a.bind_team_check, 0);
            addBool(a.bind_fov_visualize, 0);
            addBool(a.bind_humanize_enable, 0);
            addBool(a.bind_humanize_rand_smooth, 0);
            addBool(a.bind_humanize_jitter, 0);
            addBool(a.bind_humanize_overshoot, 0);
            addBool(a.bind_humanize_undershoot, 0);
            addBool(a.bind_humanize_curve, 0);
            addBool(a.bind_humanize_reaction_enable, 0);
            for (int s = 0; s < 3; ++s) {
                addValue(a.bind_fov.slots[s], 0, 0.0f, 180.0f);
                addValue(a.bind_smooth.slots[s], 0, 0.0f, 100.0f);
                addValue(a.bind_hum_smooth_min.slots[s], 0, 0.0f, 100.0f);
                addValue(a.bind_hum_smooth_max.slots[s], 0, 0.0f, 100.0f);
                addValue(a.bind_hum_jitter_amount.slots[s], 0, 0.0f, 20.0f);
                addValue(a.bind_hum_overshoot_min.slots[s], 0, 0.0f, 50.0f);
                addValue(a.bind_hum_overshoot_max.slots[s], 0, 0.0f, 50.0f);
                addValue(a.bind_hum_undershoot_min.slots[s], 0, 0.0f, 50.0f);
                addValue(a.bind_hum_undershoot_max.slots[s], 0, 0.0f, 50.0f);
                addValue(a.bind_hum_curve_amount.slots[s], 0, 0.0f, 1.0f);
                addValue(a.bind_hum_reaction_min.slots[s], 0, 0.0f, 500.0f);
                addValue(a.bind_hum_reaction_max.slots[s], 0, 0.0f, 500.0f);
            }
        }

        for (int side = 0; side < 2; ++side) {
            for (int i = 0; i < Config::kBindEspItemCount; ++i) {
                if (i == 33 || i == 34) continue;
                addBool(config.bind_esp[side][i], 1 + side);
            }
            for (int s = 0; s < 3; ++s) {
                addValue(config.bind_esp_offscreen_dist[side].slots[s], 1 + side, 0.15f, 0.95f);
                addValue(config.bind_esp_offscreen_size[side].slots[s], 1 + side, 12.0f, 50.0f);
            }
        }

        addBool(config.bind_vsync, 3);
        addBool(config.bind_allow_tearing, 3);
        addBool(config.bind_fullscreen_mode, 3);
        addBool(config.bind_streamproof, 3);
        addBool(config.bind_safe_mode, 3);

        std::stable_sort(out.begin(), out.end(),
            [](const ActiveBindRow& a, const ActiveBindRow& b) { return a.slot < b.slot; });
    }

    static ImVec2 s_pos = ImVec2(-1.0f, -1.0f);
    static bool   s_dragging = false;
    static ImVec2 s_dragDelta = ImVec2(0.0f, 0.0f);
    static float  s_rowHover[512] = {};

    void Render(float s, ImVec2 menuPos, float menuW) {
        std::vector<ActiveBindRow> rows;
        CollectRows(rows);
        if (rows.empty()) return;

        if (s_pos.x < 0.0f || s_pos.y < 0.0f) {
            s_pos = ImVec2(menuPos.x + menuW + 12.0f, menuPos.y + 60.0f);
        }

        const float width = 256.0f * s;
        const float pad = 10.0f * s;
        const float headerH = 40.0f * s;
        const float rowH = 24.0f * s;
        const float rowGap = 2.0f * s;
        const float bottomPad = 8.0f * s;

        const float contentH =
            rowH * (float)rows.size() +
            rowGap * (float)((rows.size() > 0) ? (rows.size() - 1) : 0);
        const float totalH = headerH + pad + contentH + bottomPad;

        ImGui::SetNextWindowPos(s_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, totalH), ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 9.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

        ImGui::Begin("##keybinds_list", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 wpos = ImGui::GetWindowPos();
        const ImVec2 wsize = ImGui::GetWindowSize();
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        ImFont* font = ImGui::GetFont();

        dl->AddRectFilled(wpos, ImVec2(wpos.x + wsize.x, wpos.y + wsize.y),
            IM_COL32(14, 14, 19, 248), 9.0f * s);

        {
            const float time = (float)ImGui::GetTime();
            const float pulseWidth = width * 0.28f;
            const float travelSpan = width + pulseWidth * 2.0f;
            const float speed = width * 0.42f;
            const float cycle = fmodf(time * speed, travelSpan);
            const float u = cycle / travelSpan;
            const float eased = u * u * (3.0f - 2.0f * u);
            const float pulsePos = eased * travelSpan - pulseWidth;

            Framework::DrawFlowLine(dl,
                ImVec2(wpos.x + 1, wpos.y + headerH),
                ImVec2(wpos.x + wsize.x - 1, wpos.y + headerH),
                IM_COL32(110, 100, 170, 85),
                IM_COL32(160, 140, 255, 220),
                IM_COL32(235, 228, 255, 220),
                1.0f, pulsePos, pulseWidth);
        }

        dl->AddRect(wpos, ImVec2(wpos.x + wsize.x, wpos.y + wsize.y),
            IM_COL32(189, 176, 255, 70), 9.0f * s, 0, 1.0f * s);

        if (font) {
            dl->AddText(font, 14.0f * s,
                ImVec2(wpos.x + 13.0f * s, wpos.y + 12.0f * s),
                IM_COL32(220, 215, 250, 255), "Keybinds");
        }

        const float titleH = 26.0f * s;
        const bool inTitle = (mouse.x >= wpos.x && mouse.x <= wpos.x + wsize.x &&
            mouse.y >= wpos.y && mouse.y <= wpos.y + titleH);

        if (!s_dragging && inTitle && ImGui::IsMouseClicked(0)) {
            s_dragging = true;
            s_dragDelta = ImVec2(mouse.x - wpos.x, mouse.y - wpos.y);
        }
        if (s_dragging) {
            if (ImGui::IsMouseDown(0)) {
                s_pos = ImVec2(mouse.x - s_dragDelta.x, mouse.y - s_dragDelta.y);
                ImGui::SetWindowPos(s_pos);
            }
            else {
                s_dragging = false;
            }
        }

        const float kApproach = 1.0f - expf(-18.0f * ImGui::GetIO().DeltaTime);
        const float contentW = width - pad * 2.0f;

        static const ImU32 kCatColor[4] = {
            IM_COL32(190, 170, 255, 255),
            IM_COL32(255, 160, 175, 255),
            IM_COL32(160, 210, 255, 255),
            IM_COL32(170, 245, 200, 255),
        };

        float y = wpos.y + headerH + pad;

        for (size_t i = 0; i < rows.size(); ++i) {
            const ActiveBindRow& r = rows[i];

            const ImVec2 rowMin(wpos.x + pad, y);
            const ImVec2 rowMax(wpos.x + pad + contentW, y + rowH);

            const bool hov = mouse.x >= rowMin.x && mouse.x <= rowMax.x &&
                mouse.y >= rowMin.y && mouse.y <= rowMax.y;

            float& hv = s_rowHover[i & 511];
            hv += ((hov ? 1.0f : 0.0f) - hv) * kApproach;

            if (hv > 0.005f) {
                dl->AddRectFilled(rowMin, rowMax,
                    IM_COL32(189, 176, 255, (int)(hv * 22)), 4.0f * s);
            }

            const float cy = (rowMin.y + rowMax.y) * 0.5f;
            const float dotX = rowMin.x + 8.0f * s;
            const float dotR = 3.0f * s;
            const int cat = (r.category >= 0 && r.category < 4) ? r.category : 0;

            if (r.hasValue) {
                dl->AddCircleFilled(ImVec2(dotX, cy), dotR * 0.9f,
                    kCatColor[cat], 12);
            }
            else if (r.isOn) {
                dl->AddCircleFilled(ImVec2(dotX, cy), dotR * 2.1f,
                    IM_COL32(189, 176, 255, 45), 16);
                dl->AddCircleFilled(ImVec2(dotX, cy), dotR,
                    IM_COL32(222, 212, 255, 255), 12);
            }
            else {
                dl->AddCircle(ImVec2(dotX, cy), dotR,
                    IM_COL32(82, 77, 108, 255), 12, 1.2f * s);
            }

            const float keyFontSz = 10.0f * s;
            float keyTextW = 0.0f;
            if (font) keyTextW = font->CalcTextSizeA(keyFontSz, FLT_MAX, 0.0f, r.keyName.c_str()).x;

            const float keyPadX = 5.5f * s;
            const float keyH = 14.5f * s;
            const float keyW = keyTextW + keyPadX * 2.0f;
            const float keyX1 = rowMax.x - 1.0f * s;
            const float keyX0 = keyX1 - keyW;
            const float keyY0 = cy - keyH * 0.5f;
            const float keyY1 = keyY0 + keyH;

            const int kbg = (int)(30 + 18 * hv);
            const int kbr = (int)(110 + 55 * hv);
            const int kbgc = (int)(100 + 55 * hv);
            const int kbb = (int)(155 + 65 * hv);

            dl->AddRectFilled(ImVec2(keyX0, keyY0), ImVec2(keyX1, keyY1),
                IM_COL32(kbg, kbg - 2, kbg + 15, 245), 3.0f * s);
            dl->AddRect(ImVec2(keyX0, keyY0), ImVec2(keyX1, keyY1),
                IM_COL32(kbr, kbgc, kbb, 130), 3.0f * s, 0, 1.0f);

            if (font) {
                ImVec2 ktsz = font->CalcTextSizeA(keyFontSz, FLT_MAX, 0.0f, r.keyName.c_str());
                dl->AddText(font, keyFontSz,
                    ImVec2(keyX0 + keyPadX, cy - ktsz.y * 0.5f),
                    IM_COL32(210, 205, 245, 255), r.keyName.c_str());
            }

            float rightEdge = keyX0 - 6.0f * s;

            if (r.hasValue) {
                char valBuf[32];
                if (r.vmax <= 1.5f) snprintf(valBuf, sizeof(valBuf), "%.2f", r.value);
                else if (r.vmax <= 5.0f) snprintf(valBuf, sizeof(valBuf), "%.1f", r.value);
                else snprintf(valBuf, sizeof(valBuf), "%.0f", r.value);

                float valW = 0.0f;
                if (font) valW = font->CalcTextSizeA(11.0f * s, FLT_MAX, 0.0f, valBuf).x;

                const float valX = rightEdge - valW;
                if (font) {
                    ImVec2 vsz = font->CalcTextSizeA(11.0f * s, FLT_MAX, 0.0f, valBuf);
                    dl->AddText(font, 11.0f * s,
                        ImVec2(valX, cy - vsz.y * 0.5f),
                        IM_COL32(200, 195, 240, 255), valBuf);
                }
                rightEdge = valX - 8.0f * s;
            }

            if (font) {
                float labelX = dotX + dotR + 7.0f * s;
                float labelMaxW = rightEdge - labelX;
                if (labelMaxW < 20.0f * s) labelMaxW = 20.0f * s;

                std::string clean = CleanLabel(r.label);

                dl->PushClipRect(
                    ImVec2(labelX, rowMin.y),
                    ImVec2(labelX + labelMaxW, rowMax.y), true);

                ImVec2 lsz = font->CalcTextSizeA(11.5f * s, FLT_MAX, 0.0f, clean.c_str());

                const int lr = (int)(190 + 32 * hv);
                const int lg = (int)(185 + 32 * hv);
                const int lb = (int)(220 + 32 * hv);

                dl->AddText(font, 11.5f * s,
                    ImVec2(labelX, cy - lsz.y * 0.5f),
                    IM_COL32(lr, lg, lb, 255), clean.c_str());

                dl->PopClipRect();
            }

            y += rowH + rowGap;
        }

        ImGui::End();
        ImGui::PopStyleColor(1);
        ImGui::PopStyleVar(3);
    }
}