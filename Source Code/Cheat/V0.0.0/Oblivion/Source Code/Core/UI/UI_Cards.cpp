#include "UI_Cards.hpp"
#include <cmath>

namespace UI {
    namespace Cards {

        static const char* kLabels[kEspItemCount] = {
            "Enable ESP",
            "Box",
            "Corner",
            "Rounded",
            "Fill",
            "Outline",
            "Skeleton",
            "Name",
            "Healthbar",
            "Distance",
            "Head Circle",
            "Snapline",
            "Viewdirection",
            "Weapon",
            "Flags",
            "Flashed",
            "Scoped",
            "Defusing",
            "Planting",
            "Bomb",
            "Armor / Helmet",
            "Reloading",
            "Money / Ping",
            "Ammo"
        };

        static const bool kIndented[kEspItemCount] = {
            false, false, true, true, true, true,
            false, false, false, false, false, false, false, false,
            false,
            true, true, true, true, true, true, true, true, true
        };

        static constexpr float kPad = 14.0f;
        static constexpr float kRowH = 24.0f;
        static constexpr float kCbSize = 16.0f;
        static constexpr float kIndent = 16.0f;
        static constexpr float kTopOffset = 8.0f;

        static constexpr float kExpIconSz = 11.0f;
        static constexpr float kExpGap = 6.0f;

        static constexpr float kColorW = 38.0f;
        static constexpr float kColorGap = 6.0f;

        float RowAlpha(int idx, float expandBox, float expandFlags) {
            if (idx >= 2 && idx <= 5) return expandBox;
            if (idx >= 15 && idx <= 23) return expandFlags;
            return 1.0f;
        }

        static float RowIndexY(int idx, float expandBox, float expandFlags) {
            if (idx <= 1) return (float)idx;
            if (idx <= 5) return 1.0f + (float)(idx - 1) * expandBox;
            if (idx <= 14) return (float)(idx - 4) + 4.0f * expandBox;
            return (10.0f + 4.0f * expandBox) + (float)(idx - 14) * expandFlags;
        }

        float RowCenterY(ImVec2 cardMin, int idx,
            float expandBox, float expandFlags, float s)
        {
            const float y = kTopOffset
                + (RowIndexY(idx, expandBox, expandFlags) + 0.5f) * kRowH;
            return cardMin.y + y * s;
        }

        bool HasColor(int idx) {
            switch (idx) {
            case 1:
            case 4:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
                return true;
            default:
                return false;
            }
        }

        Rect GetCheckboxRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
            float expandBox, float expandFlags, float s)
        {
            const float cbSize = kCbSize * s;
            const float pad = kPad * s;
            const float cy = RowCenterY(cardMin, idx, expandBox, expandFlags, s);
            const float x1 = cardMax.x - pad;
            const float x0 = x1 - cbSize;
            const float y0 = cy - cbSize * 0.5f;
            const float y1 = y0 + cbSize;
            return { ImVec2(x0, y0), ImVec2(x1, y1) };
        }

        Rect GetColorRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
            float expandBox, float expandFlags, float s)
        {
            const float cbSize = kCbSize * s;
            const float colorW = kColorW * s;
            const float pad = kPad * s;
            const float gap = kColorGap * s;
            const float cy = RowCenterY(cardMin, idx, expandBox, expandFlags, s);
            const float x1 = cardMax.x - pad - cbSize - gap;
            const float x0 = x1 - colorW;
            const float y0 = cy - cbSize * 0.5f;
            const float y1 = y0 + cbSize;
            return { ImVec2(x0, y0), ImVec2(x1, y1) };
        }

        static Rect MakeExpanderRect(ImVec2 cardMin, int rowIdx,
            float expandBox, float expandFlags, float s,
            ImFont* font, float labelFs, const char* label)
        {
            const float pad = kPad * s;
            const float iconSz = kExpIconSz * s;
            const float cy = RowCenterY(cardMin, rowIdx, expandBox, expandFlags, s);
            const float lx = cardMin.x + pad;

            float textW = 0.0f;
            if (font)
                textW = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, label).x;
            else
                textW = labelFs * 2.2f;

            const float cx = lx + textW + kExpGap * s + iconSz * 0.5f;
            const float half = iconSz * 0.75f;

            return {
                ImVec2(cx - half, cy - half),
                ImVec2(cx + half, cy + half)
            };
        }

        Rect GetExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
            float expandBox, float expandFlags, ImFont* font, float labelFs)
        {
            (void)cardMax;
            return MakeExpanderRect(cardMin, 1, expandBox, expandFlags, s,
                font, labelFs, "Box");
        }

        Rect GetFlagsExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
            float expandBox, ImFont* font, float labelFs)
        {
            (void)cardMax;
            return MakeExpanderRect(cardMin, 14, expandBox, 1.0f, s,
                font, labelFs, "Flags");
        }

        static void DrawChevron(ImDrawList* dl, ImVec2 c, float size,
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

        void Draw(ImDrawList* dl, ImVec2 min, ImVec2 max, float alpha,
            const float* hover, const float* sel,
            const float* colorHover, const float* const* colors,
            float expandBox, float expanderBoxHover,
            float expandFlags, float expanderFlagsHover,
            ImFont* font, float labelFs, float s,
            bool drawCardBg)
        {
            if (drawCardBg) {
                const int a8 = (int)(alpha * 255.0f);
                const int borderA = (int)(alpha * 70.0f);
                dl->AddRectFilled(min, max, IM_COL32(26, 26, 34, a8), 6.0f);
                dl->AddRect(min, max, IM_COL32(189, 176, 255, borderA),
                    6.0f, 0, 1.0f);
            }

            const float pad = kPad * s;
            const float cbRadius = 4.0f * s;
            const float iconSz = kExpIconSz * s;

            for (int i = 0; i < kEspItemCount; ++i) {
                const float rowA = RowAlpha(i, expandBox, expandFlags);
                if (rowA < 0.01f) continue;

                const float rowAlpha = alpha * rowA;

                Rect cb = GetCheckboxRect(min, max, i, expandBox, expandFlags, s);

                const float hv = hover[i];
                const float sv = sel[i];
                const float st = (sv > hv) ? sv : hv;

                const int rC = (int)(120 + (189 - 120) * st);
                const int gC = (int)(110 + (176 - 110) * st);
                const int bC = (int)(180 + (255 - 180) * st);

                const int fillA = (int)(rowAlpha * (0.35f + 0.65f * sv) * 255.0f);
                const int ringA = (int)(rowAlpha * 0.55f * hv * 255.0f);

                dl->AddRectFilled(cb.min, cb.max,
                    IM_COL32(rC, gC, bC, fillA), cbRadius);

                if (ringA > 2) {
                    dl->AddRect(cb.min, cb.max,
                        IM_COL32(rC, gC, bC, ringA),
                        cbRadius, 0, 1.5f * s);
                }

                if (HasColor(i) && colors && colors[i]) {
                    Rect cr = GetColorRect(min, max, i, expandBox, expandFlags, s);

                    const float chv = colorHover ? colorHover[i] : 0.0f;

                    const float* c = colors[i];
                    const int cR = (int)(c[0] * 255.0f);
                    const int cG = (int)(c[1] * 255.0f);
                    const int cB = (int)(c[2] * 255.0f);
                    const int cA = (int)(c[3] * 255.0f * rowAlpha);

                    dl->AddRectFilled(cr.min, cr.max,
                        IM_COL32(16, 16, 16, (int)(rowAlpha * 255.0f)), cbRadius);
                    dl->AddRectFilled(cr.min, cr.max,
                        IM_COL32(cR, cG, cB, cA), cbRadius);

                    const int bR = (int)(100 + (189 - 100) * chv);
                    const int bG = (int)(100 + (176 - 100) * chv);
                    const int bB = (int)(100 + (255 - 100) * chv);
                    const int bA = (int)(rowAlpha * (0.40f + 0.60f * chv) * 255.0f);

                    dl->AddRect(cr.min, cr.max,
                        IM_COL32(bR, bG, bB, bA),
                        cbRadius, 0, 1.0f * s);
                }

                if (font) {
                    const char* label = kLabels[i];
                    const ImVec2 lsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, label);

                    float indent = kIndented[i] ? kIndent * s : 0.0f;

                    const float lx = min.x + pad + indent;
                    const float cy = RowCenterY(min, i, expandBox, expandFlags, s);
                    const float ly = cy - lsz.y * 0.5f;

                    const int lr = (int)(190 + (235 - 190) * st);
                    const int lg = (int)(185 + (228 - 185) * st);
                    const int lb = (int)(215 + (255 - 215) * st);

                    dl->AddText(font, labelFs,
                        ImVec2(floorf(lx + 0.5f), floorf(ly + 0.5f)),
                        IM_COL32(lr, lg, lb, (int)(rowAlpha * 255.0f)), label);
                }
            }

            {
                Rect ex = GetExpanderRect(min, max, s, expandBox, expandFlags,
                    font, labelFs);
                const ImVec2 c((ex.min.x + ex.max.x) * 0.5f,
                    (ex.min.y + ex.max.y) * 0.5f);
                const float ev = expanderBoxHover;
                const int eR = (int)(150 + (189 - 150) * ev);
                const int eG = (int)(140 + (176 - 140) * ev);
                const int eB = (int)(205 + (255 - 205) * ev);
                const ImU32 eCol = IM_COL32(eR, eG, eB, (int)(alpha * 255.0f));
                const float angle = (1.5707963f) * expandBox;
                DrawChevron(dl, c, iconSz, angle, eCol, 1.6f * s);
            }

            {
                Rect fex = GetFlagsExpanderRect(min, max, s, expandBox,
                    font, labelFs);
                const ImVec2 c((fex.min.x + fex.max.x) * 0.5f,
                    (fex.min.y + fex.max.y) * 0.5f);
                const float ev = expanderFlagsHover;
                const int eR = (int)(150 + (189 - 150) * ev);
                const int eG = (int)(140 + (176 - 140) * ev);
                const int eB = (int)(205 + (255 - 205) * ev);
                const ImU32 eCol = IM_COL32(eR, eG, eB, (int)(alpha * 255.0f));
                const float angle = (1.5707963f) * expandFlags;
                DrawChevron(dl, c, iconSz, angle, eCol, 1.6f * s);
            }
        }

    }
}