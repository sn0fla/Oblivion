#include "UI_Header.hpp"
#include "UI_Internal.hpp"
#include <cmath>
#include <algorithm>

namespace UI {
    namespace Header {

        using namespace Internal;

        static const char* kVisualsSubTabs[4] = { "Enemy", "Team", "Local", "World" };
        static constexpr int kVisualsSubCount = 4;

        void Draw(ImDrawList* dl, ImVec2 pos, float menuW, float hdrH,
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
                DrawFlowLine(dl,
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

            float subWidths[4] = {};
            float totalSubW = 0.0f;
            if (subFs > 4.0f) {
                for (int i = 0; i < kVisualsSubCount; ++i) {
                    ImVec2 sz = font->CalcTextSizeA(subFs, FLT_MAX, 0.0f, kVisualsSubTabs[i]);
                    subWidths[i] = sz.x + subPadX * 2.0f;
                    totalSubW += subWidths[i];
                    if (i < kVisualsSubCount - 1) totalSubW += subGap;
                }
            }

            const float visualSelT = g_iconSel[1];
            const int   selectedIdx = 1;

            const float baseTotalW = 5.0f * boxSz + 4.0f * iconGap;
            const float insertedW = visualSelT * (totalSubW + iconGap);

            const float iconStartX = pos.x + menuW - rightMargin - baseTotalW - insertedW;
            const float iconCY = pos.y + hdrH * 0.5f;

            const float subStartX = iconStartX
                + (selectedIdx + 1) * boxSz
                + (selectedIdx + 1) * iconGap;

            auto iconOffset = [&](int i) -> float {
                float o = (float)i * (boxSz + iconGap);
                if (i > selectedIdx) o += insertedW;
                return o;
                };

            const ImVec2 mouse = io.MousePos;
            int hoverIdx = -1;
            int hoverSub = -1;

            if (selectedIdx == g_activeTab && visualSelT > 0.35f) {
                float scanX = subStartX;
                const float by0 = iconCY - boxSz * 0.42f;
                const float by1 = iconCY + boxSz * 0.42f;

                for (int i = 0; i < kVisualsSubCount; ++i) {
                    const float bx0 = scanX;
                    const float bx1 = scanX + subWidths[i];

                    if (mouse.x >= bx0 && mouse.x <= bx1 &&
                        mouse.y >= by0 && mouse.y <= by1) {
                        hoverSub = i;
                        anyIconHover = true;
                        if (ImGui::IsMouseClicked(0)) {
                            if (g_activeSub != i) PlayClickSound();
                            g_activeSub = i;
                        }
                    }
                    scanX += subWidths[i] + subGap;
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
                        if (g_activeTab != i) PlayClickSound();
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

            for (int i = 0; i < kVisualsSubCount; ++i) {
                const float th = (i == hoverSub) ? 1.0f : 0.0f;
                const float ts = (i == g_activeSub && selectedIdx == g_activeTab) ? 1.0f : 0.0f;
                g_subHover[i] += (th - g_subHover[i]) * kApproach;
                g_subSel[i] += (ts - g_subSel[i]) * kApproach;
            }

            const float glyphFs = boxSz * 0.58f;

            for (int i = 0; i < 5; ++i) {
                const float ix0 = iconStartX + iconOffset(i);
                const float ix1 = ix0 + boxSz;
                const float cx = ix0 + boxSz * 0.5f;
                const float iy0 = iconCY - boxSz * 0.5f;
                const float iy1 = iconCY + boxSz * 0.5f;

                const float state = (std::max)(g_iconSel[i], g_iconHover[i]);

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

                DrawIconGlyph(dl, i, ImVec2(cx, iconCY), boxSz, glyphFs, iconCol);

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

            if (visualSelT > 0.01f && subFs > 4.0f) {
                float drawSubX = subStartX;
                const float sBy0 = iconCY - boxSz * 0.42f;
                const float sBy1 = iconCY + boxSz * 0.42f;

                for (int i = 0; i < kVisualsSubCount; ++i) {
                    const float bx0 = drawSubX;
                    const float bx1 = drawSubX + subWidths[i];
                    const float cx = (bx0 + bx1) * 0.5f;
                    const float cy = (sBy0 + sBy1) * 0.5f;

                    const float st = (std::max)(g_subSel[i], g_subHover[i]);
                    const float bgA = (g_subSel[i] * 0.18f + g_subHover[i] * 0.08f) * visualSelT;

                    if (bgA > 0.005f) {
                        dl->AddRectFilled(
                            ImVec2(bx0, sBy0), ImVec2(bx1, sBy1),
                            IM_COL32(189, 176, 255, (int)(bgA * ta)),
                            (sBy1 - sBy0) * 0.24f);
                    }

                    const char* label = kVisualsSubTabs[i];
                    const ImVec2 lsz = font->CalcTextSizeA(subFs, FLT_MAX, 0.0f, label);
                    const float tx = floorf(cx - lsz.x * 0.5f + 0.5f);
                    const float ty = floorf(cy - lsz.y * 0.5f + 0.5f);

                    const int rC = (int)(150 + (235 - 150) * st);
                    const int gC = (int)(140 + (228 - 140) * st);
                    const int bC = (int)(205 + (255 - 205) * st);
                    const int aC = (int)(ta * visualSelT * (0.70f + 0.30f * st));

                    const ImU32 txtCol = IM_COL32(rC, gC, bC, aC);
                    dl->AddText(font, subFs, ImVec2(tx, ty), txtCol, label);

                    if (g_subSel[i] > 0.01f) {
                        const float uH = 2.0f * s;
                        const float uW = subWidths[i] * 0.55f * g_subSel[i] * visualSelT;
                        const float uY = sBy1 + 1.0f * s;
                        dl->AddRectFilled(
                            ImVec2(cx - uW * 0.5f, uY),
                            ImVec2(cx + uW * 0.5f, uY + uH),
                            IM_COL32(189, 176, 255, (int)(g_subSel[i] * ta * visualSelT)),
                            uH * 0.5f);
                    }

                    drawSubX += subWidths[i] + subGap;
                }
            }
        }

    }
}