#include "Widgets.hpp"
#include "../Framework/Framework.hpp"
#include <cmath>
#include <algorithm>

namespace Widgets {

    static void DrawAimbotIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float r = s * 0.38f;
        dl->AddCircle(c, r, col, 32, 1.8f);
        dl->AddCircleFilled(c, s * 0.10f, col, 12);
        float g0 = r + 2.5f;
        float g1 = s * 0.55f;
        dl->AddLine(ImVec2(c.x - g1, c.y), ImVec2(c.x - g0, c.y), col, 1.8f);
        dl->AddLine(ImVec2(c.x + g0, c.y), ImVec2(c.x + g1, c.y), col, 1.8f);
        dl->AddLine(ImVec2(c.x, c.y - g1), ImVec2(c.x, c.y - g0), col, 1.8f);
        dl->AddLine(ImVec2(c.x, c.y + g0), ImVec2(c.x, c.y + g1), col, 1.8f);
    }

    static void DrawEyeIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float w = s * 0.50f;
        float h = s * 0.40f;
        dl->AddBezierCubic(
            ImVec2(c.x - w, c.y),
            ImVec2(c.x - w * 0.35f, c.y - h),
            ImVec2(c.x + w * 0.35f, c.y - h),
            ImVec2(c.x + w, c.y), col, 1.8f, 24);
        dl->AddBezierCubic(
            ImVec2(c.x - w, c.y),
            ImVec2(c.x - w * 0.35f, c.y + h),
            ImVec2(c.x + w * 0.35f, c.y + h),
            ImVec2(c.x + w, c.y), col, 1.8f, 24);
        dl->AddCircleFilled(c, s * 0.14f, col, 16);
    }

    static void DrawGearIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float r = s * 0.28f;
        float outer = s * 0.48f;
        int teeth = 8;
        for (int i = 0; i < teeth; ++i) {
            float a = i * (6.2831853f / teeth);
            ImVec2 p0(c.x + cosf(a) * (r - 1.0f), c.y + sinf(a) * (r - 1.0f));
            ImVec2 p1(c.x + cosf(a) * outer, c.y + sinf(a) * outer);
            dl->AddLine(p0, p1, col, 2.4f);
        }
        dl->AddCircle(c, r, col, 32, 1.8f);
        dl->AddCircleFilled(c, r * 0.40f, col, 12);
    }

    static void DrawSlidersIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float w = s * 0.48f;
        float h1 = -s * 0.24f;
        float h2 = 0.0f;
        float h3 = s * 0.24f;

        dl->AddLine(ImVec2(c.x - w, c.y + h1), ImVec2(c.x + w, c.y + h1), col, 1.6f);
        dl->AddCircleFilled(ImVec2(c.x + w * 0.30f, c.y + h1), 2.6f, col, 12);

        dl->AddLine(ImVec2(c.x - w, c.y + h2), ImVec2(c.x + w, c.y + h2), col, 1.6f);
        dl->AddCircleFilled(ImVec2(c.x - w * 0.30f, c.y + h2), 2.6f, col, 12);

        dl->AddLine(ImVec2(c.x - w, c.y + h3), ImVec2(c.x + w, c.y + h3), col, 1.6f);
        dl->AddCircleFilled(ImVec2(c.x + w * 0.10f, c.y + h3), 2.6f, col, 12);
    }

    void DrawCategoryIcon(ImDrawList* draw, int index, ImVec2 center, float size, ImU32 color) {
        switch (index) {
        case 0: DrawAimbotIcon(draw, center, size, color);  break;
        case 1: DrawEyeIcon(draw, center, size, color);     break;
        case 2: DrawGearIcon(draw, center, size, color);    break;
        case 3: DrawSlidersIcon(draw, center, size, color); break;
        }
    }

    static void DrawEnemyIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float r = s * 0.36f;
        dl->AddCircle(c, r, col, 32, 1.5f);
        dl->AddCircleFilled(c, s * 0.09f, col, 12);
        float g0 = r * 0.55f;
        float g1 = r * 1.55f;
        dl->AddLine(ImVec2(c.x - g1, c.y), ImVec2(c.x - g0, c.y), col, 1.4f);
        dl->AddLine(ImVec2(c.x + g0, c.y), ImVec2(c.x + g1, c.y), col, 1.4f);
        dl->AddLine(ImVec2(c.x, c.y - g1), ImVec2(c.x, c.y - g0), col, 1.4f);
        dl->AddLine(ImVec2(c.x, c.y + g0), ImVec2(c.x, c.y + g1), col, 1.4f);
    }

    static void DrawTeamIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float headR = s * 0.20f;
        ImVec2 h1(c.x - s * 0.18f, c.y - s * 0.12f);
        ImVec2 h2(c.x + s * 0.18f, c.y - s * 0.12f);
        dl->AddCircle(h1, headR, col, 20, 1.5f);
        dl->AddCircle(h2, headR, col, 20, 1.5f);

        dl->AddBezierCubic(
            ImVec2(c.x - s * 0.42f, c.y + s * 0.42f),
            ImVec2(c.x - s * 0.38f, c.y + s * 0.10f),
            ImVec2(c.x + s * 0.05f, c.y + s * 0.10f),
            ImVec2(c.x + s * 0.02f, c.y + s * 0.42f),
            col, 1.5f, 20);

        dl->AddBezierCubic(
            ImVec2(c.x - s * 0.02f, c.y + s * 0.42f),
            ImVec2(c.x - s * 0.05f, c.y + s * 0.10f),
            ImVec2(c.x + s * 0.38f, c.y + s * 0.10f),
            ImVec2(c.x + s * 0.42f, c.y + s * 0.42f),
            col, 1.5f, 20);
    }

    static void DrawLocalIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float headR = s * 0.15f;
        ImVec2 head(c.x, c.y - s * 0.16f);
        dl->AddCircle(head, headR, col, 24, 1.5f);

        dl->AddBezierCubic(
            ImVec2(c.x - s * 0.32f, c.y + s * 0.38f),
            ImVec2(c.x - s * 0.30f, c.y + s * 0.05f),
            ImVec2(c.x + s * 0.30f, c.y + s * 0.05f),
            ImVec2(c.x + s * 0.32f, c.y + s * 0.38f),
            col, 1.5f, 24);
    }

    static void DrawWorldIcon(ImDrawList* dl, ImVec2 c, float s, ImU32 col) {
        float r = s * 0.38f;
        dl->AddCircle(c, r, col, 36, 1.5f);
        dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), col, 1.3f);

        dl->AddBezierCubic(
            ImVec2(c.x, c.y - r),
            ImVec2(c.x + r * 0.55f, c.y - r * 0.40f),
            ImVec2(c.x + r * 0.55f, c.y + r * 0.40f),
            ImVec2(c.x, c.y + r),
            col, 1.3f, 24);

        dl->AddBezierCubic(
            ImVec2(c.x, c.y - r),
            ImVec2(c.x - r * 0.55f, c.y - r * 0.40f),
            ImVec2(c.x - r * 0.55f, c.y + r * 0.40f),
            ImVec2(c.x, c.y + r),
            col, 1.3f, 24);

        dl->AddBezierCubic(
            ImVec2(c.x - r, c.y - r * 0.30f),
            ImVec2(c.x - r * 0.20f, c.y - r * 0.18f),
            ImVec2(c.x + r * 0.20f, c.y - r * 0.18f),
            ImVec2(c.x + r, c.y - r * 0.30f),
            col, 1.3f, 24);

        dl->AddBezierCubic(
            ImVec2(c.x - r, c.y + r * 0.30f),
            ImVec2(c.x - r * 0.20f, c.y + r * 0.18f),
            ImVec2(c.x + r * 0.20f, c.y + r * 0.18f),
            ImVec2(c.x + r, c.y + r * 0.30f),
            col, 1.3f, 24);
    }

    void DrawSubIcon(ImDrawList* draw, int index, ImVec2 center, float size, ImU32 color) {
        switch (index) {
        case 0: DrawEnemyIcon(draw, center, size, color); break;
        case 1: DrawTeamIcon(draw, center, size, color);  break;
        case 2: DrawLocalIcon(draw, center, size, color); break;
        case 3: DrawWorldIcon(draw, center, size, color); break;
        }
    }

    void DrawCheckbox(ImDrawList* draw, const Rect& box, float fillAnim,
        ImU32 accent, ImU32 border, int alpha)
    {
        draw->AddRectFilled(box.min, box.max, IM_COL32(16, 16, 16, alpha), 0.0f);
        draw->AddRect(box.min, box.max, border, 0.0f, 0, 1.0f);

        if (fillAnim > 0.001f) {
            const float fullSize = 8.0f;
            const float s = fullSize * fillAnim;
            const float cx = (box.min.x + box.max.x) * 0.5f;
            const float cy = (box.min.y + box.max.y) * 0.5f;
            const float half = s * 0.5f;
            draw->AddRectFilled(
                ImVec2(cx - half, cy - half),
                ImVec2(cx + half, cy + half),
                accent, 0.0f);
        }
    }

    static const char* kLabels[kEspItemCount] = {
        "Enable ESP",
        "Box",
        "Corner",
        "Rounded",
        "Fill",
        "Outline",
        "Skeleton",
        "Head",
        "Torso",
        "Left Arm",
        "Right Arm",
        "Left Leg",
        "Right Leg",
        "Feet",
        "Draw Joints",
        "Name",
        "Healthbar",
        "Distance",
        "Headcircle",
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
        "Ammo",
        "Offscreen",
        "Distance",
        "Size"
    };

    static const bool kIndented[kEspItemCount] = {
        false,
        false,
        true, true, true, true,
        false,
        true, true, true, true, true, true, true, true,
        false, false, false, false, false, false, false,
        false,
        true, true, true, true, true, true, true, true, true,
        false,
        true, true
    };

    static constexpr float kPad = 14.0f;
    static constexpr float kRowH = 24.0f;
    static constexpr float kSliderRowH = 16.0f;
    static constexpr float kCbSize = 16.0f;
    static constexpr float kIndent = 16.0f;
    static constexpr float kTopOffset = 8.0f;
    static constexpr float kExpIconSz = 11.0f;
    static constexpr float kExpGap = 6.0f;
    static constexpr float kColorW = 38.0f;
    static constexpr float kColorGap = 6.0f;

    float RowAlpha(int idx, float expandBox, float expandSkel, float expandFlags, float expandOffscreen) {
        if (idx >= 2 && idx <= 5) return expandBox;
        if (idx >= 7 && idx <= 14) return expandSkel;
        if (idx >= 23 && idx <= 31) return expandFlags;
        if (idx == 33 || idx == 34) return expandOffscreen;
        return 1.0f;
    }

    float RowOffsetY(int idx, float expandBox, float expandSkel, float expandFlags, float expandOffscreen) {
        float y = kTopOffset;
        if (idx == 0) return y;
        y += kRowH;
        if (idx == 1) return y;
        y += kRowH;
        for (int i = 2; i <= 5; ++i) {
            if (idx == i) return y;
            y += kRowH * expandBox;
        }
        if (idx == 6) return y;
        y += kRowH;
        for (int i = 7; i <= 14; ++i) {
            if (idx == i) return y;
            y += kRowH * expandSkel;
        }
        for (int i = 15; i <= 21; ++i) {
            if (idx == i) return y;
            y += kRowH;
        }
        if (idx == 22) return y;
        y += kRowH;
        for (int i = 23; i <= 31; ++i) {
            if (idx == i) return y;
            y += kRowH * expandFlags;
        }
        if (idx == 32) return y;
        y += kRowH;
        for (int i = 33; i <= 34; ++i) {
            if (idx == i) return y;
            y += (kRowH + kSliderRowH) * expandOffscreen;
        }
        return y;
    }

    float RowCenterY(ImVec2 cardMin, int idx,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, float s)
    {
        const float y = RowOffsetY(idx, expandBox, expandSkel, expandFlags, expandOffscreen) + kRowH * 0.5f;
        return cardMin.y + y * s;
    }

    float ContentHeight(float expandBox, float expandSkel, float expandFlags, float expandOffscreen) {
        return RowOffsetY(34, expandBox, expandSkel, expandFlags, expandOffscreen)
            + (kRowH + kSliderRowH) * expandOffscreen + 14.0f;
    }

    bool HasColor(int idx) {
        switch (idx) {
        case 1:
        case 4:
        case 6:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
            return true;
        default:
            return false;
        }
    }

    bool IsSliderRow(int idx) {
        return idx == 33 || idx == 34;
    }

    bool IsSkeletonJoint(int idx) {
        return idx >= 7 && idx <= 13;
    }

    Rect GetCheckboxRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, float s)
    {
        const float cbSize = kCbSize * s;
        const float pad = kPad * s;
        const float cy = RowCenterY(cardMin, idx, expandBox, expandSkel, expandFlags, expandOffscreen, s);
        const float x1 = cardMax.x - pad;
        const float x0 = x1 - cbSize;
        const float y0 = cy - cbSize * 0.5f;
        return { ImVec2(x0, y0), ImVec2(x1, y0 + cbSize) };
    }

    Rect GetColorRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, float s)
    {
        const float cbSize = kCbSize * s;
        const float colorW = kColorW * s;
        const float pad = kPad * s;
        const float gap = kColorGap * s;
        const float cy = RowCenterY(cardMin, idx, expandBox, expandSkel, expandFlags, expandOffscreen, s);
        const float x1 = cardMax.x - pad - cbSize - gap;
        const float x0 = x1 - colorW;
        const float y0 = cy - cbSize * 0.5f;
        return { ImVec2(x0, y0), ImVec2(x1, y0 + cbSize) };
    }

    Rect GetSliderRect(ImVec2 cardMin, ImVec2 cardMax, int idx,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen, float s)
    {
        const float pad = kPad * s;
        const float indent = kIndent * s;
        const float yOff = RowOffsetY(idx, expandBox, expandSkel, expandFlags, expandOffscreen);
        const float x0 = cardMin.x + pad + indent;
        const float x1 = cardMax.x - pad;
        const float y0 = cardMin.y + (yOff + kRowH - 3.0f) * s;
        const float y1 = y0 + (kSliderRowH - 6.0f) * s;
        return { ImVec2(x0, y0), ImVec2(x1, y1) };
    }

    static Rect MakeExpanderRect(ImVec2 cardMin, int rowIdx,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen,
        float s, ImFont* font, float labelFs, const char* label)
    {
        const float pad = kPad * s;
        const float iconSz = kExpIconSz * s;
        const float cy = RowCenterY(cardMin, rowIdx, expandBox, expandSkel, expandFlags, expandOffscreen, s);
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
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen,
        ImFont* font, float labelFs)
    {
        (void)cardMax;
        return MakeExpanderRect(cardMin, 1, expandBox, expandSkel, expandFlags, expandOffscreen,
            s, font, labelFs, "Box");
    }

    Rect GetSkelExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen,
        ImFont* font, float labelFs)
    {
        (void)cardMax;
        return MakeExpanderRect(cardMin, 6, expandBox, expandSkel, expandFlags, expandOffscreen,
            s, font, labelFs, "Skeleton");
    }

    Rect GetFlagsExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen,
        ImFont* font, float labelFs)
    {
        (void)cardMax;
        return MakeExpanderRect(cardMin, 22, expandBox, expandSkel, expandFlags, expandOffscreen,
            s, font, labelFs, "Flags");
    }

    Rect GetOffscreenExpanderRect(ImVec2 cardMin, ImVec2 cardMax, float s,
        float expandBox, float expandSkel, float expandFlags, float expandOffscreen,
        ImFont* font, float labelFs)
    {
        (void)cardMax;
        return MakeExpanderRect(cardMin, 32, expandBox, expandSkel, expandFlags, expandOffscreen,
            s, font, labelFs, "Offscreen");
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

    void DrawEspCards(ImDrawList* dl, ImVec2 min, ImVec2 max, float alpha,
        const float* hover, const float* sel,
        const float* colorHover, const float* const* colors,
        float expandBox, float expanderBoxHover,
        float expandSkel, float expanderSkelHover,
        float expandFlags, float expanderFlagsHover,
        float expandOffscreen, float expanderOffscreenHover,
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
            const float rowA = RowAlpha(i, expandBox, expandSkel, expandFlags, expandOffscreen);
            if (rowA < 0.01f) continue;

            const float rowAlpha = alpha * rowA;

            if (IsSliderRow(i)) {
                if (font) {
                    const char* label = kLabels[i];
                    const float indent = kIndented[i] ? kIndent * s : 0.0f;
                    const float lx = min.x + pad + indent;
                    const float cy = RowCenterY(min, i, expandBox, expandSkel, expandFlags, expandOffscreen, s);
                    const ImVec2 lsz = font->CalcTextSizeA(
                        labelFs, FLT_MAX, 0.0f, label);
                    const float ly = cy - lsz.y * 0.5f;

                    dl->AddText(font, labelFs,
                        ImVec2(floorf(lx + 0.5f), floorf(ly + 0.5f)),
                        IM_COL32(200, 195, 235, (int)(rowAlpha * 255.0f)),
                        label);
                }
                continue;
            }

            Rect cb = GetCheckboxRect(min, max, i, expandBox, expandSkel, expandFlags, expandOffscreen, s);

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
                Rect cr = GetColorRect(min, max, i, expandBox, expandSkel, expandFlags, expandOffscreen, s);

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
                const float cy = RowCenterY(min, i, expandBox, expandSkel, expandFlags, expandOffscreen, s);
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
            Rect ex = GetExpanderRect(min, max, s, expandBox, expandSkel,
                expandFlags, expandOffscreen, font, labelFs);
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
            Rect sex = GetSkelExpanderRect(min, max, s, expandBox, expandSkel,
                expandFlags, expandOffscreen, font, labelFs);
            const ImVec2 c((sex.min.x + sex.max.x) * 0.5f,
                (sex.min.y + sex.max.y) * 0.5f);
            const float ev = expanderSkelHover;
            const int eR = (int)(150 + (189 - 150) * ev);
            const int eG = (int)(140 + (176 - 140) * ev);
            const int eB = (int)(205 + (255 - 205) * ev);
            const ImU32 eCol = IM_COL32(eR, eG, eB, (int)(alpha * 255.0f));
            const float angle = (1.5707963f) * expandSkel;
            DrawChevron(dl, c, iconSz, angle, eCol, 1.6f * s);
        }

        {
            Rect fex = GetFlagsExpanderRect(min, max, s, expandBox, expandSkel,
                expandFlags, expandOffscreen, font, labelFs);
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

        {
            Rect oex = GetOffscreenExpanderRect(min, max, s, expandBox, expandSkel,
                expandFlags, expandOffscreen, font, labelFs);
            const ImVec2 c((oex.min.x + oex.max.x) * 0.5f,
                (oex.min.y + oex.max.y) * 0.5f);
            const float ev = expanderOffscreenHover;
            const int eR = (int)(150 + (189 - 150) * ev);
            const int eG = (int)(140 + (176 - 140) * ev);
            const int eB = (int)(205 + (255 - 205) * ev);
            const ImU32 eCol = IM_COL32(eR, eG, eB, (int)(alpha * 255.0f));
            const float angle = (1.5707963f) * expandOffscreen;
            DrawChevron(dl, c, iconSz, angle, eCol, 1.6f * s);
        }
    }

    void DrawDashedSlider(ImDrawList* dl, const Rect& r, float* val,
        float vmin, float vmax, float alpha, float s, const char* id)
    {
        const float w = r.max.x - r.min.x;
        const float h = r.max.y - r.min.y;
        if (w < 8.0f || h < 4.0f) return;

        ImGui::SetCursorScreenPos(r.min);
        ImGui::InvisibleButton(id, ImVec2(w, h));

        const bool active = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered();
        const ImGuiID myId = ImGui::GetItemID();

        static ImGuiID s_lastSliderId = 0;
        if (ImGui::IsItemClicked()) {
            s_lastSliderId = myId;
        }

        if (hovered || active) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        if (active) {
            const float mx = ImGui::GetIO().MousePos.x;
            float t = (mx - r.min.x) / w;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            *val = vmin + (vmax - vmin) * t;
        }

        const bool isFocused = (s_lastSliderId == myId) || active;
        if (isFocused && !ImGui::GetIO().WantTextInput) {
            const float range = vmax - vmin;
            const bool ctrl = ImGui::GetIO().KeyCtrl;
            const bool shift = ImGui::GetIO().KeyShift;

            float step = range * 0.01f;
            if (ctrl)       step = range * 0.001f;
            else if (shift) step = range * 0.05f;
            if (step < 1e-6f) step = 1e-6f;

            bool changed = false;
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
                *val -= step;
                changed = true;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
                *val += step;
                changed = true;
            }
            if (changed) {
                if (*val < vmin) *val = vmin;
                if (*val > vmax) *val = vmax;
            }
        }

        float t = (*val - vmin) / (vmax - vmin);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        const float cy = (r.min.y + r.max.y) * 0.5f;
        const float trackH = (std::max)(1.5f, 2.0f * s);
        const float trackY0 = cy - trackH * 0.5f;
        const float trackY1 = cy + trackH * 0.5f;

        const int a8 = (int)(alpha * 255.0f);

        dl->AddRectFilled(
            ImVec2(r.min.x, trackY0),
            ImVec2(r.max.x, trackY1),
            IM_COL32(28, 28, 38, a8), trackH * 0.5f);

        const float fillX = r.min.x + w * t;
        dl->AddRectFilled(
            ImVec2(r.min.x, trackY0),
            ImVec2(fillX, trackY1),
            IM_COL32(189, 176, 255, a8), trackH * 0.5f);

        const float filledW = fillX - r.min.x;

        if (active && filledW > 4.0f) {
            const float time = (float)ImGui::GetTime();

            const int kPulses = 3;
            const float pulseWidth = filledW * 0.55f;
            const float travelSpan = filledW + pulseWidth;
            const float speed = travelSpan * 0.9f;
            const float baseCycle = fmodf(time * speed, travelSpan);

            const ImU32 baseCol = IM_COL32(0, 0, 0, 0);
            const ImU32 glowCol = IM_COL32(160, 140, 255, (int)(alpha * 255.0f));
            const ImU32 coreCol = IM_COL32(235, 228, 255, (int)(alpha * 255.0f));

            for (int p = 0; p < kPulses; ++p) {
                const float offset = (travelSpan / (float)kPulses) * (float)p;
                const float cycle = fmodf(baseCycle + offset, travelSpan);
                const float u = cycle / travelSpan;
                const float eased = u * u * (3.0f - 2.0f * u);
                const float pulsePos = eased * travelSpan - pulseWidth;

                Framework::DrawFlowLine(dl,
                    ImVec2(r.min.x, cy),
                    ImVec2(fillX, cy),
                    baseCol, glowCol, coreCol,
                    (std::max)(1.0f, 1.0f * s), pulsePos, pulseWidth);
            }
        }

        const float handleR = (std::max)(1.8f, (active || hovered) ? 3.2f * s : 2.6f * s);
        dl->AddCircleFilled(ImVec2(fillX, cy), handleR,
            IM_COL32(235, 228, 255, a8), 16);
    }
}