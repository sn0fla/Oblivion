#include "Animations.hpp"
#include "../../Render/Render.hpp"
#include <imgui.h>
#include <cmath>

static float g_BootBlur = 0.0f;
static float g_BootContent = 0.0f;

enum BootStage {
    Boot_BlurIn,
    Boot_TextIn,
    Boot_TextHold,
    Boot_FadeOut,
    Boot_Done
};

static BootStage g_BootStage = Boot_Done;
static float g_BootTimer = 0.0f;

void Animations::BootStart() {
    g_BootStage = Boot_BlurIn;
    g_BootTimer = 0.0f;
    g_BootBlur = 0.0f;
    g_BootContent = 0.0f;
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

    auto smooth = [](float t) {
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        return t * t * (3.0f - 2.0f * t);
        };

    switch (g_BootStage) {
    case Boot_BlurIn:
        g_BootBlur = smooth(g_BootTimer / 0.70f);
        if (g_BootTimer >= 0.70f) {
            g_BootBlur = 1.0f;
            g_BootStage = Boot_TextIn;
            g_BootTimer = 0.0f;
        }
        break;
    case Boot_TextIn:
        g_BootContent = smooth(g_BootTimer / 0.55f);
        if (g_BootTimer >= 0.55f) {
            g_BootContent = 1.0f;
            g_BootStage = Boot_TextHold;
            g_BootTimer = 0.0f;
        }
        break;
    case Boot_TextHold:
        if (g_BootTimer >= 0.85f) {
            g_BootStage = Boot_FadeOut;
            g_BootTimer = 0.0f;
        }
        break;
    case Boot_FadeOut: {
        float k = 1.0f - g_BootTimer / 0.65f;
        if (k < 0.0f) k = 0.0f;
        g_BootBlur = k;
        g_BootContent = k;
        if (g_BootTimer >= 0.65f) {
            g_BootBlur = 0.0f;
            g_BootContent = 0.0f;
            g_BootStage = Boot_Done;
            g_BootTimer = 0.0f;
        }
        break;
    }
    case Boot_Done:
        break;
    }
}

static void BootRender(ImDrawList* draw, ImVec2 ds) {
    if (g_BootStage == Boot_Done) return;
    if (g_BootBlur <= 0.001f && g_BootContent <= 0.001f) return;

    if (g_BootBlur > 0.001f) {
        int blurA = (int)(g_BootBlur * 235.0f);
        if (blurA > 255) blurA = 255;
        if (blurA < 0) blurA = 0;
        draw->AddRectFilled(ImVec2(0.0f, 0.0f), ds, IM_COL32(6, 6, 10, blurA));

        int vgA = (int)(g_BootBlur * 110.0f);
        if (vgA > 0) {
            const float bandH = ds.y * 0.35f;
            draw->AddRectFilledMultiColor(
                ImVec2(0.0f, 0.0f), ImVec2(ds.x, bandH),
                IM_COL32(0, 0, 0, vgA), IM_COL32(0, 0, 0, vgA),
                IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
            draw->AddRectFilledMultiColor(
                ImVec2(0.0f, ds.y - bandH), ImVec2(ds.x, ds.y),
                IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0),
                IM_COL32(0, 0, 0, vgA), IM_COL32(0, 0, 0, vgA));
        }
    }

    if (g_BootContent <= 0.001f) return;

    ImFont* font = Render::BoldFont();
    if (!font) font = ImGui::GetFont();

    const char* title = "Oblivion";
    const float fontSize = 72.0f;
    const ImVec2 titleSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, title);

    const float cx = ds.x * 0.5f;
    const float cy = ds.y * 0.5f;
    const float tx = cx - titleSize.x * 0.5f;
    const float ty = cy - titleSize.y * 0.5f;

    int ta = (int)(g_BootContent * 255.0f);
    if (ta > 255) ta = 255;
    if (ta < 0) ta = 0;

    const ImU32 glowOuter = IM_COL32(140, 120, 255, ta / 3);
    const ImU32 glowInner = IM_COL32(180, 165, 255, ta / 2);
    const ImU32 coreText = IM_COL32(220, 210, 255, ta);

    const float off = 1.5f;
    draw->AddText(font, fontSize, ImVec2(tx - off, ty), glowOuter, title);
    draw->AddText(font, fontSize, ImVec2(tx + off, ty), glowOuter, title);
    draw->AddText(font, fontSize, ImVec2(tx, ty - off), glowOuter, title);
    draw->AddText(font, fontSize, ImVec2(tx, ty + off), glowOuter, title);

    draw->AddText(font, fontSize, ImVec2(tx - 0.5f, ty), glowInner, title);
    draw->AddText(font, fontSize, ImVec2(tx + 0.5f, ty), glowInner, title);

    draw->AddText(font, fontSize, ImVec2(tx, ty), coreText, title);
}

void Animations::Render() {
    if (g_BootStage == Boot_Done) return;
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    BootRender(fg, ImGui::GetIO().DisplaySize);
}