#include "Animations.hpp"
#include "../../Render/Render.hpp"
#include <imgui.h>
#include <cmath>

static float g_BootBlur = 0.0f;
static float g_BootContent = 0.0f;
static float g_BootSpinTime = 0.0f;

enum BootStage {
    Boot_BlurIn,
    Boot_TextIn,
    Boot_Spinner,
    Boot_FadeOut,
    Boot_Hold,
    Boot_Done
};

static BootStage g_BootStage = Boot_Done;
static float g_BootTimer = 0.0f;

static inline float Snap(float v) { return std::floor(v + 0.5f); }

void Animations::BootStart() {
    g_BootStage = Boot_BlurIn;
    g_BootTimer = 0.0f;
    g_BootBlur = 0.0f;
    g_BootContent = 0.0f;
    g_BootSpinTime = 0.0f;
}

float Animations::BootGetBlurAlpha() {
    return g_BootBlur;
}

bool Animations::IsBooting() {
    return g_BootStage != Boot_Done;
}

void Animations::BootTick() {
    if (g_BootStage == Boot_Done) return;

    float dt = ImGui::GetIO().DeltaTime;
    if (dt > 0.05f) dt = 0.05f;
    if (dt < 0.0f)  dt = 0.0f;

    g_BootTimer += dt;

    if (g_BootStage == Boot_Spinner || g_BootStage == Boot_TextIn)
        g_BootSpinTime += dt * 3.0f;

    auto smooth = [](float t) {
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        return t * t * (3.0f - 2.0f * t);
        };

    switch (g_BootStage) {
    case Boot_BlurIn:
        g_BootBlur = smooth(g_BootTimer / 0.55f);
        if (g_BootTimer >= 0.55f) {
            g_BootBlur = 1.0f;
            g_BootStage = Boot_TextIn;
            g_BootTimer = 0.0f;
        }
        break;
    case Boot_TextIn:
        g_BootContent = smooth(g_BootTimer / 0.50f);
        if (g_BootTimer >= 0.50f) {
            g_BootContent = 1.0f;
            g_BootStage = Boot_Spinner;
            g_BootTimer = 0.0f;
        }
        break;
    case Boot_Spinner:
        if (g_BootTimer >= 1.75f) {
            g_BootStage = Boot_FadeOut;
            g_BootTimer = 0.0f;
        }
        break;
    case Boot_FadeOut: {
        float k = 1.0f - g_BootTimer / 0.60f;
        if (k < 0.0f) k = 0.0f;
        g_BootBlur = k;
        g_BootContent = k;
        if (g_BootTimer >= 0.60f) {
            g_BootBlur = 0.0f;
            g_BootContent = 0.0f;
            g_BootStage = Boot_Hold;
            g_BootTimer = 0.0f;
        }
        break;
    }
    case Boot_Hold:
        if (g_BootTimer >= 1.0f) {
            g_BootStage = Boot_Done;
            g_BootTimer = 0.0f;
        }
        break;
    case Boot_Done:
        break;
    }
}

static void BootDrawFadingArc(ImDrawList* draw, ImVec2 center, float radius,
    float startAngle, float endAngle, float thickness, ImU32 baseCol, float alphaMul)
{
    const int SEG = 64;
    if (endAngle <= startAngle) return;
    int baseA = (int)(((baseCol >> 24) & 0xFF) * alphaMul);
    if (baseA <= 0) return;
    ImU32 rgb = baseCol & 0x00FFFFFF;
    ImVec2 prev;
    for (int i = 0; i <= SEG; ++i) {
        float t = (float)i / (float)SEG;
        float a = startAngle + (endAngle - startAngle) * t;
        ImVec2 cur(center.x + cosf(a) * radius, center.y + sinf(a) * radius);
        if (i > 0) {
            int segA = (int)(baseA * (t * t));
            if (segA < 0) segA = 0;
            ImU32 segCol = rgb | ((ImU32)segA << 24);
            draw->AddLine(prev, cur, segCol, thickness);
        }
        prev = cur;
    }
}

static void BootDrawSpinner(ImDrawList* draw, ImVec2 center, float radius,
    float time, ImU32 col)
{
    const float TAU = 6.28318530718f;
    {
        float startA = time * 1.6f;
        float span = TAU * 0.72f;
        BootDrawFadingArc(draw, center, radius, startA, startA + span, 3.0f, col, 1.0f);
    }
    {
        float startA = -time * 1.15f;
        float span = TAU * 0.48f;
        BootDrawFadingArc(draw, center, radius * 0.66f, startA, startA + span, 2.4f, col, 0.75f);
    }
    {
        float startA = time * 2.4f;
        float span = TAU * 0.30f;
        BootDrawFadingArc(draw, center, radius * 0.36f, startA, startA + span, 2.0f, col, 0.55f);
    }
    {
        int a8 = (int)((col >> 24) & 0xFF);
        if (a8 > 0) {
            ImU32 dotCol = (col & 0x00FFFFFF) | ((ImU32)a8 << 24);

            float a1 = -time * 0.9f;
            float r1 = radius * 1.06f;
            draw->AddCircleFilled(ImVec2(center.x + cosf(a1) * r1,
                center.y + sinf(a1) * r1), 2.6f, dotCol, 12);

            float a2 = time * 1.05f + 2.3f;
            float r2 = radius * 0.86f;
            int a2_8 = (int)(a8 * 0.6f);
            ImU32 dotCol2 = (col & 0x00FFFFFF) | ((ImU32)a2_8 << 24);
            draw->AddCircleFilled(ImVec2(center.x + cosf(a2) * r2,
                center.y + sinf(a2) * r2), 2.0f, dotCol2, 12);
        }
    }
}

static void BootRender(ImDrawList* draw, ImVec2 ds) {
    if (g_BootStage == Boot_Done) return;
    if (g_BootContent <= 0.001f) return;

    int a8 = (int)(g_BootContent * 255.0f);
    if (a8 > 255) a8 = 255;
    if (a8 < 0) a8 = 0;

    ImU32 accent = IM_COL32(255, 50, 50, a8);
    ImU32 dim = IM_COL32(200, 200, 200, a8);

    ImFont* font = Render::BoldFont();
    if (!font) font = ImGui::GetFont();

    float cx = ds.x * 0.5f;
    float cy = ds.y * 0.5f;

    float spinR = 26.0f;

    const char* status = "Initializing";
    const float statusFs = 20.0f;
    ImVec2 stSz = font->CalcTextSizeA(statusFs, FLT_MAX, 0.0f, status);

    float gap = 22.0f;
    float groupW = spinR * 2.0f + gap + stSz.x;
    float groupX = cx - groupW * 0.5f;
    float lineY = cy + 30.0f;

    ImVec2 spinCenter(groupX + spinR, lineY);
    BootDrawSpinner(draw, spinCenter, spinR, g_BootSpinTime, accent);

    ImVec2 textPos(groupX + spinR * 2.0f + gap, Snap(lineY - stSz.y * 0.5f));
    draw->AddText(font, statusFs, textPos, dim, status);
}

void Animations::Render() {
    if (g_BootStage == Boot_Done) return;
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    BootRender(fg, ImGui::GetIO().DisplaySize);
}