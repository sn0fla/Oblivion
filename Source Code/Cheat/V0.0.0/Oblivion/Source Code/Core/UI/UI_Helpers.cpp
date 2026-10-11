#include "UI_Internal.hpp"
#include <Windows.h>
#include <mmsystem.h>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

namespace UI {
    namespace Internal {

        ImU32 ScaleAlpha(ImU32 col, float mul) {
            int a = (int)(((col >> 24) & 0xFF) * mul);
            if (a < 0) a = 0;
            if (a > 255) a = 255;
            return (col & 0x00FFFFFF) | ((ImU32)a << 24);
        }

        float Hash01(unsigned int x) {
            x = (x ^ 61u) ^ (x >> 16);
            x = x + (x << 3);
            x = x ^ (x >> 4);
            x = x * 0x27d4eb2du;
            x = x ^ (x >> 15);
            return (float)(x & 0xFFFFFFu) / (float)0xFFFFFFu;
        }

        void PlayClickSound() {
            static const char kPath[] = "C:\\Oblivion\\Resources\\Sounds\\Click.wav";
            PlaySoundA(kPath, nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
        }

        static ImFont* g_iconFont = nullptr;

        static const ImWchar kIconRanges[] = {
            0xE700, 0xE7FF,
            0xE800, 0xE8FF,
            0xE900, 0xE9FF,
            0xEA00, 0xEAFF,
            0x0000
        };

        ImFont* GetIconFont() {
            if (g_iconFont) return g_iconFont;
            ImGuiIO& io = ImGui::GetIO();
            ImFontConfig cfg;
            cfg.OversampleH = 4;
            cfg.OversampleV = 4;
            cfg.PixelSnapH = false;

            const char* candidates[] = {
                "C:\\Windows\\Fonts\\segmdl2.ttf",
                "C:\\Windows\\Fonts\\SegoeIcons.ttf",
                "C:\\Windows\\Fonts\\seguisym.ttf",
            };
            for (const char* path : candidates) {
                g_iconFont = io.Fonts->AddFontFromFileTTF(path, 32.0f, &cfg, kIconRanges);
                if (g_iconFont) break;
            }
            return g_iconFont;
        }

        static const char* kIconGlyphs[5] = {
            nullptr,
            "\xEE\x9D\xBB",
            "\xEE\x9C\x93",
            "\xEE\x9F\x83",
            "\xEE\x9F\xAF",
        };

        void DrawFlowLine(ImDrawList* dl, ImVec2 a, ImVec2 b,
            ImU32 baseCol, ImU32 glowCol, ImU32 coreCol,
            float thickness, float pulsePos, float pulseWidth)
        {
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float len = sqrtf(dx * dx + dy * dy);
            if (len <= 0.0f) return;

            dl->AddLine(a, b, baseCol, thickness);

            const float ux = dx / len;
            const float uy = dy / len;
            const float inv = 1.0f / (pulseWidth > 0.0f ? pulseWidth : 1.0f);

            const int steps = (int)(len / 2.0f);
            if (steps < 2) return;

            const float stepLen = len / (float)steps;

            for (int pass = 0; pass < 3; ++pass) {
                ImU32 col;
                float width;
                float falloffExp;

                if (pass == 0) {
                    col = glowCol;
                    width = thickness * 3.2f;
                    falloffExp = 3.2f;
                }
                else if (pass == 1) {
                    col = glowCol;
                    width = thickness * 1.7f;
                    falloffExp = 1.8f;
                }
                else {
                    col = coreCol;
                    width = thickness * 1.0f;
                    falloffExp = 1.1f;
                }

                for (int i = 0; i < steps; ++i) {
                    const float t = ((float)i + 0.5f) * stepLen;
                    const float d = fabsf(t - pulsePos) * inv;
                    if (d >= 1.0f) continue;

                    const float f = powf(1.0f - d, falloffExp);
                    if (f < 0.01f) continue;

                    const float th = width * (pass == 0 ? (1.0f - f * 0.35f) : 1.0f);
                    const ImU32 c = ScaleAlpha(col, f * 0.9f);

                    const float x0 = a.x + ux * t;
                    const float y0 = a.y + uy * t;
                    const float x1 = a.x + ux * (t + stepLen);
                    const float y1 = a.y + uy * (t + stepLen);

                    dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), c, th);
                }
            }
        }

        void DrawIconGlyph(ImDrawList* dl, int type, ImVec2 c,
            float boxSz, float fontSz, ImU32 col)
        {
            if (type == 0) {
                const float r = boxSz * 0.24f;
                const float arm = boxSz * 0.14f;
                const float t = (std::max)(1.0f, boxSz * 0.048f);
                dl->AddCircle(c, r, col, 24, t);
                dl->AddLine(ImVec2(c.x - r - arm, c.y),
                    ImVec2(c.x - r * 0.35f, c.y), col, t);
                dl->AddLine(ImVec2(c.x + r * 0.35f, c.y),
                    ImVec2(c.x + r + arm, c.y), col, t);
                dl->AddLine(ImVec2(c.x, c.y - r - arm),
                    ImVec2(c.x, c.y - r * 0.35f), col, t);
                dl->AddLine(ImVec2(c.x, c.y + r * 0.35f),
                    ImVec2(c.x, c.y + r + arm), col, t);
                return;
            }

            ImFont* f = GetIconFont();
            if (!f) return;
            const char* g = kIconGlyphs[type];
            if (!g) return;

            const ImVec2 sz = f->CalcTextSizeA(fontSz, FLT_MAX, 0.0f, g);
            dl->AddText(f, fontSz,
                ImVec2(c.x - sz.x * 0.5f, c.y - sz.y * 0.5f),
                col, g);
        }

    }
}