#include "UI.hpp"
#include "UI_Internal.hpp"
#include "UI_Header.hpp"
#include "UI_Stars.hpp"
#include "UI_Cards.hpp"
#include "../../SDK/Entities/Entities.hpp"
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

        float g_espHover[2][27] = {};
        float g_espSel[2][27] = {};
        float g_espColorHover[2][27] = {};

        bool  g_espExpanded[2] = { false, false };
        float g_espExpandT[2] = { 0.0f, 0.0f };
        float g_espExpanderHover[2] = { 0.0f, 0.0f };

        bool  g_flagsExpanded[2] = { false, false };
        float g_flagsExpandT[2] = { 0.0f, 0.0f };
        float g_flagsExpanderHover[2] = { 0.0f, 0.0f };

        bool  g_offscreenExpanded[2] = { false, false };
        float g_offscreenExpandT[2] = { 0.0f, 0.0f };
        float g_offscreenExpanderHover[2] = { 0.0f, 0.0f };
    }

    static float  g_linearAnim = 0.0f;
    static ImVec2 g_menuPos = ImVec2(-1.0f, -1.0f);
    static bool   g_dragging = false;
    static ImVec2 g_dragDelta = ImVec2(0.0f, 0.0f);

    static float  g_safeModeHover = 0.0f;
    static float  g_unloadHover = 0.0f;
    static float  g_diagHover = 0.0f;
    static bool   g_showDiagnostics = false;
    static bool   g_requestSafeModeConfirm = false;
    static bool   g_requestUnloadConfirm = false;

    static constexpr float kMenuAnimDur = 0.10f;

    namespace Diag {
        static constexpr int kHistSize = 120;

        static float    s_frameHist[kHistSize] = {};
        static int      s_frameIdx = 0;

        static double   s_accumMs = 0.0;
        static uint64_t s_accumCount = 0;
        static double   s_frameAvgMs = 0.0;
        static double   s_frameMinMs = 0.0;
        static double   s_frameMaxMs = 0.0;
        static float    s_fps = 0.0f;

        static double   s_prevPubTime = 0.0;
        static double   s_snapshotAgeMs = 0.0;
        static double   s_ageAccum = 0.0;
        static uint64_t s_ageCount = 0;
        static double   s_snapshotRateHz = 0.0;
        static uint64_t s_snapTickCount = 0;
        static double   s_rateWindowStart = 0.0;

        static double   s_uiAccum = 0.0;
        static double   s_vmAccum = 0.0;
        static double   s_swAccum = 0.0;
        static double   s_espAccum = 0.0;
        static double   s_cbAccum = 0.0;
        static double   s_ageAtRenderAccum = 0.0;
        static uint64_t s_pipeCount = 0;

        static double   s_uiRenderAvgMs = 0.0;
        static double   s_vmReadAvgMs = 0.0;
        static double   s_snapWaitAvgMs = 0.0;
        static double   s_espRenderAvgMs = 0.0;
        static double   s_renderCbAvgMs = 0.0;
        static double   s_ageAtRenderAvgMs = 0.0;
        static int      s_entityCount = 0;

        static double   s_overlayPresentAccum = 0.0;
        static double   s_overlayFrameAccum = 0.0;
        static double   s_overlayPresentAvgMs = 0.0;
        static double   s_overlayFrameAvgMs = 0.0;

        static ImU32 ColorForMs(double ms, double goodMs, double badMs) {
            if (ms <= goodMs) return IM_COL32(140, 230, 150, 255);
            if (ms >= badMs)  return IM_COL32(240, 130, 130, 255);
            return IM_COL32(240, 210, 130, 255);
        }

        static void Tick(float dt) {
            if (dt <= 0.0f) return;

            const double ms = static_cast<double>(dt) * 1000.0;
            s_fps = static_cast<float>(1.0 / static_cast<double>(dt));

            s_frameHist[s_frameIdx] = static_cast<float>(ms);
            s_frameIdx = (s_frameIdx + 1) % kHistSize;

            s_accumMs += ms;
            s_accumCount++;
            if (s_accumMs > 500.0) {
                s_frameAvgMs = s_accumMs / static_cast<double>(s_accumCount);
                s_accumMs = 0.0;
                s_accumCount = 0;
            }

            double mn = 1e9, mx = 0.0;
            for (int i = 0; i < kHistSize; ++i) {
                const float v = s_frameHist[i];
                if (v <= 0.0f) continue;
                if (static_cast<double>(v) < mn) mn = static_cast<double>(v);
                if (static_cast<double>(v) > mx) mx = static_cast<double>(v);
            }
            s_frameMinMs = (mn < 1e8) ? mn : 0.0;
            s_frameMaxMs = mx;

            const double now = FrameTime();
            const double pub = GetLastSnapshotPublishTime();

            if (pub > 0.0 && pub != s_prevPubTime) {
                s_prevPubTime = pub;
                s_snapTickCount++;
                if (s_rateWindowStart == 0.0) s_rateWindowStart = now;

                const double span = now - s_rateWindowStart;
                if (span >= 0.5) {
                    s_snapshotRateHz = static_cast<double>(s_snapTickCount) / span;
                    s_snapTickCount = 0;
                    s_rateWindowStart = now;
                }
            }

            if (pub > 0.0) {
                const double age = (now - pub) * 1000.0;
                s_ageAccum += age;
                s_ageCount++;
                if (s_ageCount >= 30) {
                    s_snapshotAgeMs = s_ageAccum / static_cast<double>(s_ageCount);
                    s_ageAccum = 0.0;
                    s_ageCount = 0;
                }
            }

            s_uiAccum += g_uiRenderMs.load(std::memory_order_relaxed);
            s_vmAccum += g_viewMatrixReadMs.load(std::memory_order_relaxed);
            s_swAccum += g_snapshotWaitMs.load(std::memory_order_relaxed);
            s_espAccum += g_espRenderMs.load(std::memory_order_relaxed);
            s_cbAccum += g_renderCallbackMs.load(std::memory_order_relaxed);
            s_ageAtRenderAccum += g_snapshotAgeAtRenderMs.load(std::memory_order_relaxed);
            s_entityCount = g_renderEntityCount.load(std::memory_order_relaxed);

            s_overlayPresentAccum += g_overlayPresentLatencyMs.load(std::memory_order_relaxed);
            s_overlayFrameAccum += g_overlayFrameTimeMs.load(std::memory_order_relaxed);

            s_pipeCount++;

            if (s_pipeCount >= 30) {
                const double n = static_cast<double>(s_pipeCount);
                s_uiRenderAvgMs = s_uiAccum / n;
                s_vmReadAvgMs = s_vmAccum / n;
                s_snapWaitAvgMs = s_swAccum / n;
                s_espRenderAvgMs = s_espAccum / n;
                s_renderCbAvgMs = s_cbAccum / n;
                s_ageAtRenderAvgMs = s_ageAtRenderAccum / n;
                s_uiAccum = 0.0;
                s_vmAccum = 0.0;
                s_swAccum = 0.0;
                s_espAccum = 0.0;
                s_cbAccum = 0.0;
                s_ageAtRenderAccum = 0.0;

                s_overlayPresentAvgMs = s_overlayPresentAccum / n;
                s_overlayFrameAvgMs = s_overlayFrameAccum / n;
                s_overlayPresentAccum = 0.0;
                s_overlayFrameAccum = 0.0;

                s_pipeCount = 0;
            }
        }

        static void Row(const char* label, const char* value, ImU32 valueCol) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 155, 195, 255));
            ImGui::TextUnformatted(label);
            ImGui::PopStyleColor();
            ImGui::SameLine(160.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, valueCol);
            ImGui::TextUnformatted(value);
            ImGui::PopStyleColor();
        }

        static void SectionHeader(const char* title) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(190, 180, 245, 255));
            ImGui::TextUnformatted(title);
            ImGui::PopStyleColor();
            ImGui::Separator();
        }

        static void RenderWindow() {
            if (!g_showDiagnostics) return;

            ImGui::SetNextWindowSize(ImVec2(380.0f, 0.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(24.0f, 24.0f), ImGuiCond_FirstUseEver);

            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.055f, 0.055f, 0.075f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.741f, 0.690f, 1.0f, 0.45f));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.08f, 0.08f, 0.11f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.10f, 0.10f, 0.14f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.86f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.741f, 0.690f, 1.0f, 0.25f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));

            if (ImGui::Begin("Diagnostics", &g_showDiagnostics,
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize)) {

                char buf[96];

                SectionHeader("Render");

                snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(s_fps));
                Row("FPS", buf, ColorForMs(s_fps > 1.0f ? 1000.0 / static_cast<double>(s_fps) : 9999.0, 8.0, 25.0));

                snprintf(buf, sizeof(buf), "%.2f ms", s_frameAvgMs);
                Row("Frame avg", buf, ColorForMs(s_frameAvgMs, 8.0, 25.0));

                snprintf(buf, sizeof(buf), "%.2f ms", s_frameMinMs);
                Row("Frame min", buf, IM_COL32(200, 195, 235, 255));

                snprintf(buf, sizeof(buf), "%.2f ms", s_frameMaxMs);
                Row("Frame max", buf, ColorForMs(s_frameMaxMs, 16.0, 40.0));

                snprintf(buf, sizeof(buf), "%.2f ms", s_renderCbAvgMs);
                Row("Render callback", buf, ColorForMs(s_renderCbAvgMs, 4.0, 14.0));

                {
                    double present = s_frameAvgMs - s_renderCbAvgMs;
                    if (present < 0.0) present = 0.0;
                    snprintf(buf, sizeof(buf), "%.2f ms", present);
                    Row("Present / other", buf, ColorForMs(present, 6.0, 16.0));
                }

                ImGui::Spacing();
                SectionHeader("Render breakdown");

                snprintf(buf, sizeof(buf), "%.2f ms", s_uiRenderAvgMs);
                Row("UI render", buf, ColorForMs(s_uiRenderAvgMs, 1.5, 5.0));

                snprintf(buf, sizeof(buf), "%.2f ms", s_vmReadAvgMs);
                Row("View matrix read", buf, ColorForMs(s_vmReadAvgMs, 0.5, 3.0));

                snprintf(buf, sizeof(buf), "%.2f ms", s_snapWaitAvgMs);
                Row("Snapshot lock wait", buf, ColorForMs(s_snapWaitAvgMs, 0.5, 3.0));

                snprintf(buf, sizeof(buf), "%.2f ms", s_espRenderAvgMs);
                Row("ESP render", buf, ColorForMs(s_espRenderAvgMs, 2.0, 8.0));

                {
                    double other = s_renderCbAvgMs - s_uiRenderAvgMs - s_vmReadAvgMs - s_snapWaitAvgMs - s_espRenderAvgMs;
                    if (other < 0.0) other = 0.0;
                    snprintf(buf, sizeof(buf), "%.2f ms", other);
                    Row("Callback other", buf, ColorForMs(other, 1.0, 4.0));
                }

                ImGui::Spacing();
                SectionHeader("Entity thread");

                const double hz = GetEntityThreadHz();
                const double build = GetEntityThreadLastBuildMs();

                snprintf(buf, sizeof(buf), "%.1f Hz", hz);
                Row("Iteration rate", buf, ColorForMs(hz > 0.0 ? 1000.0 / hz : 9999.0, 8.0, 25.0));

                snprintf(buf, sizeof(buf), "%.3f ms", build);
                Row("Build time", buf, ColorForMs(build, 3.0, 12.0));

                {
                    const uint64_t iters = GetEntityThreadIterations();
                    snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(iters));
                    Row("Total iterations", buf, IM_COL32(200, 195, 235, 255));
                }

                snprintf(buf, sizeof(buf), "%d", s_entityCount);
                Row("Entities in frame", buf, IM_COL32(200, 195, 235, 255));

                ImGui::Spacing();
                SectionHeader("Snapshot");

                snprintf(buf, sizeof(buf), "%.1f Hz", s_snapshotRateHz);
                Row("Publish rate", buf, ColorForMs(s_snapshotRateHz > 0.0 ? 1000.0 / s_snapshotRateHz : 9999.0, 8.0, 25.0));

                snprintf(buf, sizeof(buf), "%.2f ms", s_ageAtRenderAvgMs);
                Row("Age at render", buf, ColorForMs(s_ageAtRenderAvgMs, 8.0, 25.0));

                snprintf(buf, sizeof(buf), "%.2f ms", s_snapshotAgeMs);
                Row("Age (UI tick)", buf, ColorForMs(s_snapshotAgeMs, 8.0, 25.0));

                ImGui::Spacing();
                SectionHeader("Overlay");

                snprintf(buf, sizeof(buf), "%.2f ms", s_overlayPresentAvgMs);
                Row("Data -> Present", buf, ColorForMs(s_overlayPresentAvgMs, 5.0, 15.0));

                snprintf(buf, sizeof(buf), "%.2f ms", s_overlayFrameAvgMs);
                Row("Overlay frame time", buf, ColorForMs(s_overlayFrameAvgMs, 4.0, 12.0));

                ImGui::Spacing();
                SectionHeader("Pipeline");

                {
                    double toEsp = s_ageAtRenderAvgMs + s_espRenderAvgMs;
                    snprintf(buf, sizeof(buf), "%.2f ms", toEsp);
                    Row("Entity -> ESP", buf, ColorForMs(toEsp, 15.0, 40.0));
                }
                {
                    double toCb = s_ageAtRenderAvgMs + s_renderCbAvgMs;
                    snprintf(buf, sizeof(buf), "%.2f ms", toCb);
                    Row("Entity -> callback end", buf, ColorForMs(toCb, 20.0, 50.0));
                }
                {
                    double toScreen = s_ageAtRenderAvgMs + s_frameAvgMs;
                    snprintf(buf, sizeof(buf), "%.2f ms", toScreen);
                    Row("Entity -> screen (est)", buf, ColorForMs(toScreen, 25.0, 60.0));
                }
                {
                    double total = s_frameAvgMs + s_snapshotAgeMs + build;
                    snprintf(buf, sizeof(buf), "%.2f ms", total);
                    Row("Legacy total", buf, ColorForMs(total, 20.0, 50.0));
                }
            }
            ImGui::End();

            ImGui::PopStyleVar(4);
            ImGui::PopStyleColor(6);
        }
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
        const float rowGap = 6.0f * s;

        const float cbSize = 18.0f * s;
        const float cbX1 = cmax.x - cp;
        const float cbX0 = cbX1 - cbSize;

        const float row1Y = cmin.y + cp;
        const float cb1Y0 = row1Y + (rowH - cbSize) * 0.5f;
        const float cb1Y1 = cb1Y0 + cbSize;

        const float row2Y = row1Y + rowH + rowGap;
        const float cb2Y0 = row2Y + (rowH - cbSize) * 0.5f;
        const float cb2Y1 = cb2Y0 + cbSize;

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const bool interactive = (a > 0.99f);

        const bool safeCbHovered = interactive
            && mouse.x >= cbX0 && mouse.x <= cbX1
            && mouse.y >= cb1Y0 && mouse.y <= cb1Y1;

        const bool diagCbHovered = interactive
            && mouse.x >= cbX0 && mouse.x <= cbX1
            && mouse.y >= cb2Y0 && mouse.y <= cb2Y1;

        const float kApproach = 1.0f - expf(-18.0f * dt);
        g_safeModeHover += ((safeCbHovered ? 1.0f : 0.0f) - g_safeModeHover) * kApproach;
        g_diagHover += ((diagCbHovered ? 1.0f : 0.0f) - g_diagHover) * kApproach;

        {
            const bool safeEnabled = config.safe_mode;
            const float safeSelT = safeEnabled ? 1.0f : 0.0f;
            const float safeSt = (safeSelT > g_safeModeHover) ? safeSelT : g_safeModeHover;

            const int mR = (int)(120 + (189 - 120) * safeSt);
            const int mG = (int)(110 + (176 - 110) * safeSt);
            const int mB = (int)(180 + (255 - 180) * safeSt);
            const int mFillA = (int)(cardAlpha * (0.35f + 0.65f * safeSelT) * 255.0f);
            const int mRingA = (int)(cardAlpha * 0.55f * g_safeModeHover * 255.0f);

            dl->AddRectFilled(ImVec2(cbX0, cb1Y0), ImVec2(cbX1, cb1Y1),
                IM_COL32(mR, mG, mB, mFillA), 4.0f * s);

            if (mRingA > 2) {
                dl->AddRect(ImVec2(cbX0, cb1Y0), ImVec2(cbX1, cb1Y1),
                    IM_COL32(mR, mG, mB, mRingA), 4.0f * s, 0, 1.5f * s);
            }

            if (font) {
                const char* lbl = "Safemode";
                const ImVec2 lsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, lbl);
                const float lx = cmin.x + cp;
                const float ly = row1Y + (rowH - lsz.y) * 0.5f;

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
        }

        {
            const bool diagEnabled = g_showDiagnostics;
            const float diagSelT = diagEnabled ? 1.0f : 0.0f;
            const float diagSt = (diagSelT > g_diagHover) ? diagSelT : g_diagHover;

            const int mR = (int)(120 + (189 - 120) * diagSt);
            const int mG = (int)(110 + (176 - 110) * diagSt);
            const int mB = (int)(180 + (255 - 180) * diagSt);
            const int mFillA = (int)(cardAlpha * (0.35f + 0.65f * diagSelT) * 255.0f);
            const int mRingA = (int)(cardAlpha * 0.55f * g_diagHover * 255.0f);

            dl->AddRectFilled(ImVec2(cbX0, cb2Y0), ImVec2(cbX1, cb2Y1),
                IM_COL32(mR, mG, mB, mFillA), 4.0f * s);

            if (mRingA > 2) {
                dl->AddRect(ImVec2(cbX0, cb2Y0), ImVec2(cbX1, cb2Y1),
                    IM_COL32(mR, mG, mB, mRingA), 4.0f * s, 0, 1.5f * s);
            }

            if (font) {
                const char* lbl = "Diagnostics";
                const ImVec2 lsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, lbl);
                const float lx = cmin.x + cp;
                const float ly = row2Y + (rowH - lsz.y) * 0.5f;

                const int lr = (int)(190 + (235 - 190) * diagSt);
                const int lg = (int)(185 + (228 - 185) * diagSt);
                const int lb = (int)(215 + (255 - 215) * diagSt);

                dl->AddText(font, labelFs,
                    ImVec2(floorf(lx + 0.5f), floorf(ly + 0.5f)),
                    IM_COL32(lr, lg, lb, (int)(cardAlpha * 255.0f)), lbl);
            }

            if (diagCbHovered && ImGui::IsMouseClicked(0)) {
                Internal::PlayClickSound();
                g_showDiagnostics = !g_showDiagnostics;
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

    static void DrawDashedSlider(ImDrawList* dl, const Cards::Rect& r, float* val,
        float vmin, float vmax, float alpha, float s, const char* id)
    {
        const float w = r.max.x - r.min.x;
        const float h = r.max.y - r.min.y;
        if (w < 8.0f || h < 4.0f) return;

        ImGui::SetCursorScreenPos(r.min);
        ImGui::InvisibleButton(id, ImVec2(w, h));

        const bool active = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered();

        if (active) {
            const float mx = ImGui::GetIO().MousePos.x;
            float t = (mx - r.min.x) / w;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            *val = vmin + (vmax - vmin) * t;
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

                Internal::DrawFlowLine(dl,
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

    static void PlaceOffscreenSliders(ImDrawList* dl, ImVec2 drawMin, ImVec2 drawMax,
        float expandBox, float expandFlags, float expandOffscreen,
        float s, int curSub, float a, ImFont* font, float labelFs)
    {
        if (a < 0.5f) return;
        if (expandOffscreen < 0.01f) return;

        static const char* s_ids[2] = { "##offs_dist", "##offs_size" };

        for (int k = 0; k < 2; ++k) {
            const int idx = 25 + k;

            const float rowA = Cards::RowAlpha(idx, expandBox, expandFlags, expandOffscreen);
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
                const float labelCy = Cards::RowCenterY(drawMin, idx, expandBox, expandFlags,
                    expandOffscreen, s);
                const ImVec2 tsz = font->CalcTextSizeA(labelFs, FLT_MAX, 0.0f, buf);
                const float tx = drawMax.x - 14.0f * s - tsz.x;
                const float ty = labelCy - tsz.y * 0.5f;
                dl->AddText(font, labelFs,
                    ImVec2(floorf(tx + 0.5f), floorf(ty + 0.5f)),
                    IM_COL32(200, 190, 240, (int)(alpha * 255.0f)), buf);
            }

            Cards::Rect sr = Cards::GetSliderRect(
                drawMin, drawMax, idx, expandBox, expandFlags, expandOffscreen, s);

            if (sr.max.x - sr.min.x < 16.0f) continue;

            DrawDashedSlider(dl, sr, valPtr, vmin, vmax, alpha, s, s_ids[k]);
        }
    }

    void Render(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height) {
        (void)device; (void)context; (void)width; (void)height;

        g_AllowMemoryWrite.store(!config.safe_mode, std::memory_order_relaxed);

        const ImGuiIO& io = ImGui::GetIO();

        float dt = io.DeltaTime;
        if (dt > 0.05f) dt = 0.05f;
        if (dt < 0.0f)  dt = 0.0f;

        Diag::Tick(dt);

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

            const float contentHeight = Cards::ContentHeight(
                Internal::g_espExpandT[curSub],
                Internal::g_flagsExpandT[curSub],
                Internal::g_offscreenExpandT[curSub]) * s;
            ImGui::Dummy(ImVec2(cardW, contentHeight));

            const float expandBox = Internal::g_espExpandT[curSub];
            const float expandFlags = Internal::g_flagsExpandT[curSub];
            const float expandOffscreen = Internal::g_offscreenExpandT[curSub];

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

            for (int i = 0; i < Cards::kEspItemCount; ++i) {
                if (Cards::IsSliderRow(i)) continue;
                const float rowA = Cards::RowAlpha(i, expandBox, expandFlags, expandOffscreen);
                const bool interactive = mouseInCard && (rowA > 0.5f);

                Cards::Rect cb = Cards::GetCheckboxRect(
                    drawMin, drawMax, i, expandBox, expandFlags, expandOffscreen, s);

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
                if (Cards::IsSliderRow(i)) continue;
                const float rowA = Cards::RowAlpha(i, expandBox, expandFlags, expandOffscreen);
                const bool interactive = mouseInCard && (rowA > 0.5f);

                bool colorHov = false;

                float* colorPtr = Cards::HasColor(i) ? GetEspColorPtr(curSub, i) : nullptr;

                if (colorPtr) {
                    Cards::Rect cr = Cards::GetColorRect(
                        drawMin, drawMax, i, expandBox, expandFlags, expandOffscreen, s);

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
                expandBox, expandFlags, expandOffscreen, font, labelFs);
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
                expandBox, expandFlags, expandOffscreen, font, labelFs);
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

            Cards::Rect oex = Cards::GetOffscreenExpanderRect(drawMin, drawMax, s,
                expandBox, expandFlags, expandOffscreen, font, labelFs);
            const bool oexHov = mouseInCard
                && mouse.x >= oex.min.x && mouse.x <= oex.max.x
                && mouse.y >= oex.min.y && mouse.y <= oex.max.y;

            if (oexHov && ImGui::IsMouseClicked(0)) {
                Internal::PlayClickSound();
                Internal::g_offscreenExpanded[curSub] = !Internal::g_offscreenExpanded[curSub];
            }

            Internal::g_offscreenExpanderHover[curSub] +=
                ((oexHov ? 1.0f : 0.0f) - Internal::g_offscreenExpanderHover[curSub]) * kApproach;

            const float targetOffscreen = Internal::g_offscreenExpanded[curSub] ? 1.0f : 0.0f;
            Internal::g_offscreenExpandT[curSub] +=
                (targetOffscreen - Internal::g_offscreenExpandT[curSub]) * kApproach;

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
                Internal::g_offscreenExpandT[curSub],
                Internal::g_offscreenExpanderHover[curSub],
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

        Diag::RenderWindow();
    }

    bool IsOpen() { return config.show_menu; }
    void Toggle() {}
}