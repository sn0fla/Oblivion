#include "Aimbot.hpp"
#include "../../../../Utilities/Config/Config.hpp"
#include "../../../../Utilities/Maths/Maths.hpp"
#include "../../../../SDK/Schemas/client_dll.hpp"
#include "../../../../SDK/Schemas/offsets.hpp"
#include "../../../Memory/Memory.hpp"
#include "../../Visuals/Player/ESP/Weapon/Weapon.hpp"
#include "../../../../External/ImGUI/imgui.h"
#include <Windows.h>
#include <cmath>
#include <cfloat>
#include <cctype>
#include <cstring>
#include <algorithm>
#include <vector>
#include <random>
#include <string>

namespace Aimbot {

    static bool s_toggled = false;
    static bool s_keyPrev = false;

    static float s_degPerPixel = 0.044f;
    static int   s_calibSamples = 0;

    static constexpr float  kMinDpp = 0.005f;
    static constexpr float  kMaxDpp = 0.5f;
    static constexpr float  kDeadzoneDeg = 0.01f;
    static constexpr float  kSampleMinPx = 6.0f;
    static constexpr float  kMaxDegPerFrame = 15.0f;
    static constexpr float  kSmoothCurve = 0.03f;
    static constexpr float  kAccumLimit = 500.0f;
    static constexpr double kMinSendGapSec = 0.005;
    static constexpr double kPendingTimeout = 0.04;
    static constexpr bool   kInvertDx = false;

    static constexpr float  kCurveRefDeg = 10.0f;
    static constexpr float  kCurveMaxRad = 0.50f;

    static constexpr float  kBurstStartDeg = 2.0f;
    static constexpr double kBurstTimeout = 0.5;
    static constexpr float  kMinVelFactor = 0.20f;
    static constexpr float  kMinCorrectionDeg = 0.30f;

    static Vector3 s_anglesBeforeSend{};
    static float   s_lastSentDx = 0.0f;
    static float   s_lastSentDy = 0.0f;
    static bool    s_pending = false;
    static double  s_pendingTime = 0.0;
    static double  s_lastSendTime = 0.0;
    static float   s_accumDx = 0.0f;
    static float   s_accumDy = 0.0f;

    static bool  s_aimWasActive = false;
    static float s_burstCurveDir = 1.0f;

    static uintptr_t s_reactTarget = 0;
    static double    s_reactStart = 0.0;
    static float     s_reactDelay = 0.0f;

    static bool   s_burstActive = false;
    static float  s_burstMagRaw = 0.0f;
    static double s_burstStart = 0.0;

    static bool   s_correctionActive = false;
    static double s_correctionStart = 0.0;
    static float  s_correctionDur = 0.0f;
    static float  s_correctionOffsetDeg = 0.0f;

    static float s_noisePhase[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    static constexpr float kNoiseFreq[4] = { 5.5f, 8.0f, 11.5f, 15.0f };
    static bool s_noiseSeeded = false;

    static std::mt19937 s_rng(0x9E3779B9u);

    static float RandFloat(float lo, float hi) {
        if (lo >= hi) return lo;
        std::uniform_real_distribution<float> d(lo, hi);
        return d(s_rng);
    }

    static void SeedNoise() {
        if (s_noiseSeeded) return;
        s_noiseSeeded = true;
        for (int i = 0; i < 4; ++i)
            s_noisePhase[i] = RandFloat(0.0f, 6.2831853f);
    }

    static float ValueNoise(double t, int axis) {
        float sum = 0.0f;
        for (int i = 0; i < 4; ++i) {
            const double phase = (double)s_noisePhase[i]
                + (axis ? 1.7 : 0.0) * (double)(i + 1);
            const double arg = t * 6.283185307179586 * (double)kNoiseFreq[i] + phase;
            sum += (float)sin(arg);
        }
        return sum * 0.25f;
    }

    static float MinJerkVel(float p) {
        if (p <= 0.0f) return 0.0f;
        if (p >= 1.0f) return 0.0f;
        const float u = 1.0f - p;
        return 16.0f * p * p * u * u;
    }

    using NtUserSendInput_t = UINT(NTAPI*)(UINT cInputs, LPINPUT pInputs, int cbSize);

    static NtUserSendInput_t s_ntSendInput = nullptr;
    static bool              s_syscallInitTried = false;

    static void* FindSyscallGadget() {
        HMODULE ntdll = GetModuleHandleA("ntdll.dll");
        if (!ntdll) return nullptr;
        auto* base = reinterpret_cast<BYTE*>(ntdll);
        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;
        auto* sec = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            if (memcmp(sec[i].Name, ".text", 5) != 0) continue;
            BYTE* start = base + sec[i].VirtualAddress;
            SIZE_T size = sec[i].Misc.VirtualSize;
            for (SIZE_T off = 0; off + 2 < size; ++off) {
                if (start[off] == 0x0F && start[off + 1] == 0x05 && start[off + 2] == 0xC3)
                    return start + off;
            }
        }
        return nullptr;
    }

    static bool ExtractSSN(BYTE* fn, DWORD& outSSN) {
        if (!fn) return false;
        for (int i = 0; i + 7 < 64; ++i) {
            if (fn[i] != 0xB8) continue;
            DWORD ssn = *reinterpret_cast<DWORD*>(fn + i + 1);
            if (ssn == 0 || ssn > 0x10000) continue;
            if (fn[i + 5] == 0x0F && fn[i + 6] == 0x05) { outSSN = ssn; return true; }
        }
        return false;
    }

    static bool InitIndirectSyscall() {
        HMODULE win32u = GetModuleHandleA("win32u.dll");
        if (!win32u) win32u = LoadLibraryA("win32u.dll");
        if (!win32u) return false;

        auto* fn = reinterpret_cast<BYTE*>(GetProcAddress(win32u, "NtUserSendInput"));
        if (!fn) return false;

        DWORD ssn = 0;
        if (!ExtractSSN(fn, ssn)) return false;

        void* gadget = FindSyscallGadget();
        if (!gadget) return false;

        if (!g_spoofRets[0]) GetNtReadVirtualMemory();

        int nFrames = 0;
        if (g_spoofRets[0]) nFrames++;
        if (g_spoofRets[1] && nFrames >= 1) nFrames++;
        if (g_spoofRets[2] && nFrames >= 2) nFrames++;

        SIZE_T codeSize = static_cast<SIZE_T>(nFrames) * 6 + 14;
        SIZE_T dataStart = ((codeSize + 7) / 8) * 8;
        SIZE_T totalSize = dataStart + 8 + static_cast<SIZE_T>(nFrames) * 8;
        totalSize = ((totalSize + 15) / 16) * 16;
        if (totalSize < 64) totalSize = 64;

        auto* buf = static_cast<BYTE*>(VirtualAlloc(nullptr, totalSize,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (!buf) return false;
        memset(buf, 0xCC, totalSize);

        SIZE_T o = 0;
        for (int i = nFrames - 1; i >= 0; --i) {
            SIZE_T pos = o;
            buf[o++] = 0xFF; buf[o++] = 0x35;
            int32_t disp = static_cast<int32_t>(
                (dataStart + 8 + static_cast<SIZE_T>(i) * 8) - (pos + 6));
            *reinterpret_cast<int32_t*>(buf + o) = disp; o += 4;
        }
        buf[o++] = 0x4C; buf[o++] = 0x8B; buf[o++] = 0xD1;
        buf[o++] = 0xB8;
        *reinterpret_cast<DWORD*>(buf + o) = ssn; o += 4;
        SIZE_T jmpPos = o;
        buf[o++] = 0xFF; buf[o++] = 0x25;
        *reinterpret_cast<int32_t*>(buf + o) =
            static_cast<int32_t>(dataStart - (jmpPos + 6)); o += 4;
        while (o < dataStart) buf[o++] = 0xCC;

        *reinterpret_cast<uintptr_t*>(buf + dataStart) = reinterpret_cast<uintptr_t>(gadget);
        for (int i = 0; i < nFrames; ++i)
            *reinterpret_cast<uintptr_t*>(buf + dataStart + 8 + static_cast<SIZE_T>(i) * 8) =
            g_spoofRets[i];

        DWORD oldProt = 0;
        if (!VirtualProtect(buf, totalSize, PAGE_EXECUTE_READ, &oldProt)) return false;
        s_ntSendInput = reinterpret_cast<NtUserSendInput_t>(buf);
        return true;
    }

    static void EnsureSyscall() {
        if (s_syscallInitTried) return;
        s_syscallInitTried = true;
        InitIndirectSyscall();
    }

    static bool ShouldAim(const AimSettings& cfg) {
        if (cfg.mode == 2) return true;
        const int key = cfg.key;
        if (key <= 0) { s_keyPrev = false; return false; }
        const bool keyDown = (GetAsyncKeyState(key) & 0x8000) != 0;
        if (cfg.mode == 0) {
            if (keyDown && !s_keyPrev) s_toggled = !s_toggled;
            s_keyPrev = keyDown;
            return s_toggled;
        }
        s_keyPrev = keyDown;
        return keyDown;
    }

    static bool IsEntitySpotted(const FrameEntity& ent, uint32_t localIndex) {
        using namespace cs2_dumper::schemas::client_dll;
        if (!ent.address) return false;
        if (localIndex == 0xFFFFFFFFu || localIndex > 63u) return false;

        const uintptr_t base = ent.address + C_CSPlayerPawn::m_entitySpottedState;
        uint64_t mask = 0;
        if (!mem.ReadRaw(base + 0x08, &mask, sizeof(mask))) return false;
        return ((mask >> (localIndex & 0x3Fu)) & 1ull) != 0ull;
    }

    static bool PickBestBone(const FrameEntity& ent, int hitboxMask,
        const Vector3& eyePos, const Vector3& viewAngles,
        Vector3& outPos, float& outFov)
    {
        if (!ent.hasBones) {
            const Vector3 fallback = ent.origin + Vector3(0.0f, 0.0f, 64.0f);
            outPos = fallback;
            outFov = Math::GetFov(viewAngles, Math::CalcAngle(eyePos, fallback));
            return true;
        }

        auto B = [&](BoneIndex idx) -> const Vector3& {
            return ent.bones[static_cast<int>(idx)];
            };

        bool    found = false;
        float   bestFov = FLT_MAX;
        Vector3 bestPos{};

        auto consider = [&](const Vector3& pos, float penalty = 0.0f) {
            if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f) return;
            const float fov = Math::GetFov(viewAngles, Math::CalcAngle(eyePos, pos)) + penalty;
            if (fov < bestFov) { bestFov = fov; bestPos = pos; found = true; }
            };

        if (hitboxMask & 1) {
            const Vector3& headBone = B(BoneIndex::Head);
            const Vector3& neckBone = B(BoneIndex::Neck);
            const bool neckValid = (neckBone.x != 0.0f || neckBone.y != 0.0f || neckBone.z != 0.0f);
            const Vector3 axis = neckValid ? (headBone - neckBone) : Vector3(0.0f, 0.0f, 8.0f);
            const float penalty = (config.aim_shared.hitbox_priority == 2) ? 15.0f : 0.0f;
            consider(headBone + axis * 0.75f, penalty);
        }
        if (hitboxMask & 2) {
            const float penalty = (config.aim_shared.hitbox_priority == 2) ? 10.0f : 0.0f;
            consider(B(BoneIndex::Neck), penalty);
        }
        if (hitboxMask & 4) {
            const float penalty = (config.aim_shared.hitbox_priority == 1) ? 10.0f : 0.0f;
            consider(B(BoneIndex::Spine_2), penalty);
        }
        if (hitboxMask & 8) {
            const float penalty = (config.aim_shared.hitbox_priority == 1) ? 15.0f : 0.0f;
            consider(B(BoneIndex::Spine_1), penalty);
        }
        if (hitboxMask & 16) {
            const float penalty = (config.aim_shared.hitbox_priority == 1) ? 15.0f : 5.0f;
            consider(B(BoneIndex::LeftUpperArm), penalty);  consider(B(BoneIndex::RightUpperArm), penalty);
            consider(B(BoneIndex::LeftForearm), penalty);   consider(B(BoneIndex::RightForearm), penalty);
            consider(B(BoneIndex::LeftHand), penalty);      consider(B(BoneIndex::RightHand), penalty);
        }
        if (hitboxMask & 32) {
            const float penalty = (config.aim_shared.hitbox_priority == 1) ? 15.0f : 5.0f;
            consider(B(BoneIndex::LeftHip), penalty);   consider(B(BoneIndex::RightHip), penalty);
            consider(B(BoneIndex::LeftKnee), penalty);  consider(B(BoneIndex::RightKnee), penalty);
        }
        if (hitboxMask & 64) {
            const float penalty = (config.aim_shared.hitbox_priority == 1) ? 20.0f : 10.0f;
            consider(B(BoneIndex::LeftFoot), penalty);  consider(B(BoneIndex::RightFoot), penalty);
        }

        if (!found) {
            const Vector3& headBone = B(BoneIndex::Head);
            const Vector3& neckBone = B(BoneIndex::Neck);
            const bool neckValid = (neckBone.x != 0.0f || neckBone.y != 0.0f || neckBone.z != 0.0f);
            const Vector3 axis = neckValid ? (headBone - neckBone) : Vector3(0.0f, 0.0f, 8.0f);
            consider(headBone + axis * 0.75f);
        }
        if (!found) return false;

        outPos = bestPos;
        outFov = bestFov;
        return true;
    }

    static float UnwrapYaw(float dy) {
        while (dy > 180.0f) dy -= 360.0f;
        while (dy < -180.0f) dy += 360.0f;
        return dy;
    }

    static void SendMouseDelta(LONG dx, LONG dy) {
        if (dx == 0 && dy == 0) return;
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = dx;
        input.mi.dy = dy;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.time = 0;
        input.mi.dwExtraInfo = 0;
        EnsureSyscall();
        if (s_ntSendInput)
            s_ntSendInput(1, &input, sizeof(INPUT));
        else
            SendInput(1, &input, sizeof(INPUT));
    }

    static void BlendCalibration(float measured) {
        if (measured < kMinDpp || measured > kMaxDpp) return;
        const float alpha = (s_calibSamples < 10) ? 0.7f : 0.15f;
        s_degPerPixel = s_degPerPixel * (1.0f - alpha) + measured * alpha;
        s_degPerPixel = std::clamp(s_degPerPixel, kMinDpp, kMaxDpp);
        if (s_calibSamples < 1000) ++s_calibSamples;
    }

    static void UpdateCalibration(const Vector3& anglesNow) {
        const float sentDx = std::fabsf(s_lastSentDx);
        const float sentDy = std::fabsf(s_lastSentDy);
        if (sentDx >= kSampleMinPx) {
            const float obsYaw = std::fabsf(UnwrapYaw(anglesNow.y - s_anglesBeforeSend.y));
            if (obsYaw > 0.02f) BlendCalibration(obsYaw / sentDx);
        }
        if (sentDy >= kSampleMinPx) {
            const float obsPitch = std::fabsf(anglesNow.x - s_anglesBeforeSend.x);
            if (obsPitch > 0.02f) BlendCalibration(obsPitch / sentDy);
        }
    }

    static bool WeaponNameMatches(const char* preset, const char* actual) {
        if (!preset || !actual || !actual[0]) return false;
        char a[64] = {};
        char p[64] = {};
        size_t ai = 0, pi = 0;
        for (const char* s = actual; *s && ai < sizeof(a) - 1; ++s) {
            unsigned char c = static_cast<unsigned char>(*s);
            if (isalnum(c)) a[ai++] = static_cast<char>(tolower(c));
        }
        for (const char* s = preset; *s && pi < sizeof(p) - 1; ++s) {
            unsigned char c = static_cast<unsigned char>(*s);
            if (isalnum(c)) p[pi++] = static_cast<char>(tolower(c));
        }
        if (ai == 0 || pi == 0) return false;
        if (strcmp(a, p) == 0) return true;
        if (strstr(a, p) != nullptr) return true;
        if (strstr(p, a) != nullptr) return true;
        return false;
    }

    void RenderFOV(const ViewMatrix& vm, const AimSettings& cfg) {
        if (!cfg.fov_visualize) return;
        if (cfg.fov <= 0.0f) return;

        ImGuiIO& io = ImGui::GetIO();
        const float W = io.DisplaySize.x;
        const float H = io.DisplaySize.y;
        if (W < 32.0f || H < 32.0f) return;

        const float sx = std::sqrt(vm[0][0] * vm[0][0] + vm[0][1] * vm[0][1] + vm[0][2] * vm[0][2]);
        const float sy = std::sqrt(vm[1][0] * vm[1][0] + vm[1][1] * vm[1][1] + vm[1][2] * vm[1][2]);
        if (!std::isfinite(sx) || !std::isfinite(sy)) return;
        if (sx < 1e-4f || sy < 1e-4f) return;
        if (sx > 100.0f || sy > 100.0f) return;

        const float aimFovDeg = std::clamp(cfg.fov, 0.1f, 85.0f);
        const float tanFov = std::tan(aimFovDeg * Math::PI / 180.0f);
        if (!std::isfinite(tanFov) || tanFov <= 0.0f) return;

        const float rx = tanFov * (W * 0.5f) * sx;
        const float ry = tanFov * (H * 0.5f) * sy;
        if (!std::isfinite(rx) || !std::isfinite(ry)) return;
        if (rx < 1.5f && ry < 1.5f) return;
        if (rx > W * 8.0f || ry > H * 8.0f) return;

        const float* c = cfg.fov_visualize_color;
        const ImU32 col = IM_COL32(
            static_cast<int>(c[0] * 255.0f + 0.5f),
            static_cast<int>(c[1] * 255.0f + 0.5f),
            static_cast<int>(c[2] * 255.0f + 0.5f),
            static_cast<int>(c[3] * 255.0f + 0.5f));

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;
        const ImVec2 center(W * 0.5f, H * 0.5f);

        if (std::fabsf(rx - ry) < 0.5f) {
            dl->AddCircle(center, (rx + ry) * 0.5f, col, 96, 1.5f);
        }
        else {
            const int seg = 96;
            ImVec2 prev(center.x + rx, center.y);
            for (int i = 1; i <= seg; ++i) {
                const float a = static_cast<float>(i) / static_cast<float>(seg) * 2.0f * Math::PI;
                ImVec2 pt(center.x + rx * cosf(a), center.y + ry * sinf(a));
                dl->AddLine(prev, pt, col, 1.5f);
                prev = pt;
            }
        }
    }

    void Tick(const FrameSnapshot& frame) {
        bool anyEnable = config.aim_shared.enable || config.aim_shared.fov_visualize;
        if (!anyEnable) {
            for (const auto& g : config.aim_groups) {
                if (g.settings.enable || g.settings.fov_visualize) { anyEnable = true; break; }
            }
        }
        if (!anyEnable) return;
        if (!mem.client || !mem.handle) return;

        SeedNoise();

        const AimSettings* active = &config.aim_shared;

        if (frame.localWeaponName[0] != '\0') {
            for (const auto& grp : config.aim_groups) {
                bool matched = false;
                for (const auto& w : grp.weapons) {
                    if (WeaponNameMatches(w.c_str(), frame.localWeaponName)) {
                        matched = true;
                        break;
                    }
                }
                if (matched) {
                    active = &grp.settings;
                    break;
                }
            }
        }

        if (active->fov_visualize)
            RenderFOV(frame.viewMatrix, *active);

        if (config.show_menu) return;
        if (!active->enable) return;
        if (!frame.localPawn || frame.localPawn < 0x10000) return;

        const bool aiming = ShouldAim(*active);

        if (aiming && !s_aimWasActive) {
            s_burstCurveDir = (RandFloat(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
        }
        s_aimWasActive = aiming;

        Vector3 viewAngles = mem.Read<Vector3>(
            mem.client + cs2_dumper::offsets::client_dll::dwViewAngles);
        if (!std::isfinite(viewAngles.x) || !std::isfinite(viewAngles.y)) return;
        viewAngles.z = 0.0f;

        const double now = FrameTime();

        if (s_pending) {
            const float dPitch = std::fabsf(viewAngles.x - s_anglesBeforeSend.x);
            const float dYaw = std::fabsf(UnwrapYaw(viewAngles.y - s_anglesBeforeSend.y));
            const bool changed = (dPitch > 0.001f) || (dYaw > 0.001f);
            const bool timedOut = (now - s_pendingTime) > kPendingTimeout;
            if (!changed && !timedOut) return;
            if (changed && !timedOut) UpdateCalibration(viewAngles);
            s_pending = false;
        }

        if (!aiming) {
            s_accumDx = 0.0f;
            s_accumDy = 0.0f;
            s_pending = false;
            s_burstActive = false;
            s_correctionActive = false;
            s_reactTarget = 0;
            return;
        }

        Vector3 eyePos = frame.localEyePos;
        if (eyePos.x == 0.0f && eyePos.y == 0.0f && eyePos.z == 0.0f)
            eyePos = frame.localOrigin + Vector3(0.0f, 0.0f, 64.0625f);

        const bool wantVis = active->visible_check;
        const bool wantTeamCheck = active->team_check;

        const FrameEntity* best = nullptr;
        Vector3             bestPos{};
        float               bestFov = (active->fov > 0.0f) ? active->fov : 180.0f;

        for (const auto& ent : frame.entities) {
            if (!ent.isAlive || ent.isDormant) continue;
            if (wantTeamCheck && ent.team == frame.localTeam) continue;
            if (ent.health == 0)               continue;

            if (wantVis && frame.localIndex != 0xFFFFFFFFu) {
                if (!IsEntitySpotted(ent, frame.localIndex)) continue;
            }

            Vector3 pos{};
            float   fov = 0.0f;
            if (!PickBestBone(ent, active->hitbox, eyePos, viewAngles, pos, fov))
                continue;
            if (fov > bestFov) continue;

            bestFov = fov;
            best = &ent;
            bestPos = pos;
        }

        if (!best) {
            s_burstActive = false;
            s_correctionActive = false;
            return;
        }

        if (best->address != s_reactTarget) {
            s_reactTarget = best->address;
            s_burstActive = false;
            s_correctionActive = false;
            s_accumDx = 0.0f;
            s_accumDy = 0.0f;
            s_pending = false;

            if (active->humanize_reaction_enable) {
                float rmin = (std::max)(0.0f, active->humanize_reaction_min) / 1000.0f;
                float rmax = (std::max)(0.0f, active->humanize_reaction_max) / 1000.0f;
                if (rmax < rmin) std::swap(rmin, rmax);
                s_reactDelay = (rmax > rmin) ? RandFloat(rmin, rmax) : rmin;
            }
            else {
                s_reactDelay = 0.0f;
            }
            s_reactStart = now;
        }

        if (active->humanize_reaction_enable) {
            if ((now - s_reactStart) < (double)s_reactDelay)
                return;
        }

        const Vector3 desired = Math::CalcAngle(eyePos, bestPos);

        float dYaw = UnwrapYaw(desired.y - viewAngles.y);
        float dPitch = desired.x - viewAngles.x;

        const float magRaw = std::sqrt(dYaw * dYaw + dPitch * dPitch);

        const bool humOn = active->humanize_enable;

        if (!s_burstActive && magRaw > kBurstStartDeg) {
            s_burstActive = true;
            s_burstStart = now;
            s_burstMagRaw = magRaw;

            s_correctionActive = false;
            s_correctionOffsetDeg = 0.0f;

            if (humOn && (active->humanize_overshoot || active->humanize_undershoot)) {
                float over = 0.0f;
                float under = 0.0f;
                if (active->humanize_overshoot) {
                    const float lo = (std::min)(active->humanize_overshoot_min, active->humanize_overshoot_max);
                    const float hi = (std::max)(active->humanize_overshoot_min, active->humanize_overshoot_max);
                    over = RandFloat(lo, hi) * 0.01f * magRaw;
                }
                if (active->humanize_undershoot) {
                    const float lo = (std::min)(active->humanize_undershoot_min, active->humanize_undershoot_max);
                    const float hi = (std::max)(active->humanize_undershoot_min, active->humanize_undershoot_max);
                    under = RandFloat(lo, hi) * 0.01f * magRaw;
                }
                const float net = over - under;
                if (std::fabsf(net) > kMinCorrectionDeg && net > -magRaw + 0.5f) {
                    s_correctionActive = true;
                    s_correctionStart = now;
                    s_correctionDur = RandFloat(0.03f, 0.07f);
                    s_correctionOffsetDeg = net;
                }
            }
        }

        if (s_correctionActive) {
            if ((now - s_correctionStart) < (double)s_correctionDur) {
                const float magRes = std::sqrt(dYaw * dYaw + dPitch * dPitch);
                if (magRes > 1e-3f) {
                    const float ny = dYaw / magRes;
                    const float np = dPitch / magRes;
                    dYaw += ny * s_correctionOffsetDeg;
                    dPitch += np * s_correctionOffsetDeg;
                }
            }
            else {
                s_correctionActive = false;
            }
        }

        float velFactor = 1.0f;
        if (s_burstActive) {
            if ((now - s_burstStart) > kBurstTimeout) {
                s_burstActive = false;
            }
            else {
                const float aimMag = std::sqrt(dYaw * dYaw + dPitch * dPitch);
                float progress = 1.0f - aimMag / s_burstMagRaw;
                if (progress < 0.0f) progress = 0.0f;
                if (progress > 1.0f) progress = 1.0f;
                velFactor = (std::max)(MinJerkVel(progress), kMinVelFactor);
                if (progress > 0.98f) s_burstActive = false;
            }
        }

        const float deadzone = (std::max)(kDeadzoneDeg, s_degPerPixel * 0.5f);
        if (std::fabsf(dYaw) < deadzone) dYaw = 0.0f;
        if (std::fabsf(dPitch) < deadzone) dPitch = 0.0f;
        if (dYaw == 0.0f && dPitch == 0.0f) return;

        float smoothVal = active->smooth;

        if (humOn && active->humanize_rand_smooth) {
            const float lo = (std::min)(active->humanize_smooth_min, active->humanize_smooth_max);
            const float hi = (std::max)(active->humanize_smooth_min, active->humanize_smooth_max);
            smoothVal = RandFloat(lo, hi);
        }

        if (smoothVal > 0.0f) {
            const float denom = smoothVal + smoothVal * smoothVal * kSmoothCurve;
            float maxStepDeg = kMaxDegPerFrame / denom;
            maxStepDeg *= velFactor;
            if (maxStepDeg < 0.05f) maxStepDeg = 0.05f;

            const float mag = std::sqrt(dYaw * dYaw + dPitch * dPitch);
            if (mag > maxStepDeg) {
                const float scale = maxStepDeg / mag;
                dYaw *= scale;
                dPitch *= scale;
            }
        }

        if (humOn && active->humanize_curve) {
            const float c = std::clamp(active->humanize_curve_amount, 0.0f, 1.0f);
            if (c > 0.0f) {
                const float mag = std::sqrt(dYaw * dYaw + dPitch * dPitch);
                if (mag > 1e-3f) {
                    const float distFactor = (std::min)(mag / kCurveRefDeg, 1.0f);
                    const float theta = s_burstCurveDir * c * distFactor * kCurveMaxRad;
                    const float ca = std::cos(theta);
                    const float sa = std::sin(theta);
                    const float ny = dYaw * ca - dPitch * sa;
                    const float np = dYaw * sa + dPitch * ca;
                    dYaw = ny;
                    dPitch = np;
                }
            }
        }

        float dxPixels = -dYaw / s_degPerPixel;
        float dyPixels = dPitch / s_degPerPixel;

        if (humOn && active->humanize_jitter) {
            const float j = (std::max)(0.0f, active->humanize_jitter_amount);
            if (j > 0.0f) {
                dxPixels += ValueNoise(now, 0) * j;
                dyPixels += ValueNoise(now, 1) * j;
            }
        }

        if (!std::isfinite(dxPixels) || !std::isfinite(dyPixels)) return;

        s_accumDx = std::clamp(s_accumDx + dxPixels, -kAccumLimit, kAccumLimit);
        s_accumDy = std::clamp(s_accumDy + dyPixels, -kAccumLimit, kAccumLimit);

        LONG dx = static_cast<LONG>(s_accumDx);
        LONG dy = static_cast<LONG>(s_accumDy);
        if (dx == 0 && dy == 0) return;

        if (now - s_lastSendTime < kMinSendGapSec) return;

        s_accumDx -= static_cast<float>(dx);
        s_accumDy -= static_cast<float>(dy);
        if (kInvertDx) dx = -dx;

        s_lastSendTime = now;
        s_anglesBeforeSend = viewAngles;
        s_lastSentDx = static_cast<float>(dx);
        s_lastSentDy = static_cast<float>(dy);
        s_pending = true;
        s_pendingTime = now;

        SendMouseDelta(dx, dy);
    }
}