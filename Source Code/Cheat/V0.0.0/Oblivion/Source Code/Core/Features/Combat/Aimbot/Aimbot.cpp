#include "Aimbot.hpp"
#include "../../../../Utilities/Config/Config.hpp"
#include "../../../../Utilities/Maths/Maths.hpp"
#include "../../../../SDK/Schemas/client_dll.hpp"
#include "../../../../SDK/Schemas/offsets.hpp"
#include "../../../Memory/Memory.hpp"
#include <Windows.h>
#include <cmath>
#include <cfloat>
#include <algorithm>

namespace Aimbot {

    static bool s_toggled = false;
    static bool s_keyPrev = false;

    static bool ShouldAim() {
        if (config.aim_mode == 2) return true;

        const int key = config.aim_key;
        if (key <= 0) { s_keyPrev = false; return false; }

        const bool keyDown = (GetAsyncKeyState(key) & 0x8000) != 0;

        if (config.aim_mode == 0) {
            if (keyDown && !s_keyPrev) s_toggled = !s_toggled;
            s_keyPrev = keyDown;
            return s_toggled;
        }
        s_keyPrev = keyDown;
        return keyDown;
    }

    static bool PickBestBone(const FrameEntity& ent,
        int hitboxMask,
        const Vector3& eyePos,
        const Vector3& viewAngles,
        Vector3& outPos,
        float& outFov)
    {
        if (!ent.hasBones) {
            const Vector3 fallback = ent.origin + Vector3(0.0f, 0.0f, 64.0f);
            const Vector3 desired = Math::CalcAngle(eyePos, fallback);
            outPos = fallback;
            outFov = Math::GetFov(viewAngles, desired);
            return true;
        }

        auto B = [&](BoneIndex idx) -> const Vector3& {
            return ent.bones[static_cast<int>(idx)];
            };

        bool found = false;
        float bestFov = FLT_MAX;
        Vector3 bestPos{};

        auto consider = [&](const Vector3& pos) {
            if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f) return;
            const Vector3 desired = Math::CalcAngle(eyePos, pos);
            const float fov = Math::GetFov(viewAngles, desired);
            if (fov < bestFov) {
                bestFov = fov;
                bestPos = pos;
                found = true;
            }
            };

        if (hitboxMask & 1)  consider(B(BoneIndex::Head));
        if (hitboxMask & 2)  consider(B(BoneIndex::Neck));
        if (hitboxMask & 4)  consider(B(BoneIndex::Spine_2));
        if (hitboxMask & 8)  consider(B(BoneIndex::Spine_1));

        if (hitboxMask & 16) {
            consider(B(BoneIndex::LeftUpperArm));
            consider(B(BoneIndex::RightUpperArm));
            consider(B(BoneIndex::LeftForearm));
            consider(B(BoneIndex::RightForearm));
            consider(B(BoneIndex::LeftHand));
            consider(B(BoneIndex::RightHand));
        }

        if (hitboxMask & 32) {
            consider(B(BoneIndex::LeftHip));
            consider(B(BoneIndex::RightHip));
            consider(B(BoneIndex::LeftKnee));
            consider(B(BoneIndex::RightKnee));
        }

        if (hitboxMask & 64) {
            consider(B(BoneIndex::LeftFoot));
            consider(B(BoneIndex::RightFoot));
        }

        if (!found) consider(B(BoneIndex::Head));
        if (!found) return false;

        outPos = bestPos;
        outFov = bestFov;
        return true;
    }

    void Tick(const FrameSnapshot& frame) {
        if (!config.aim_enable) return;
        if (config.show_menu)   return;
        if (!mem.client || !mem.handle) return;
        if (!ShouldAim())       return;

        const uintptr_t localPawn = mem.Read<uintptr_t>(
            mem.client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn || localPawn < 0x10000) return;

        Vector3 viewAngles = mem.Read<Vector3>(
            mem.client + cs2_dumper::offsets::client_dll::dwViewAngles);
        if (!std::isfinite(viewAngles.x) || !std::isfinite(viewAngles.y)) return;
        viewAngles.z = 0.0f;

        Vector3 eyePos = frame.localEyePos;
        if (eyePos.x == 0.0f && eyePos.y == 0.0f && eyePos.z == 0.0f) {
            eyePos = frame.localOrigin + Vector3(0.0f, 0.0f, 64.06f);
        }

        const FrameEntity* best = nullptr;
        Vector3 bestPos{};
        float bestFov = (config.aim_fov > 0.0f) ? config.aim_fov : 180.0f;

        for (const auto& ent : frame.entities) {
            if (!ent.isAlive || ent.isDormant) continue;
            if (ent.team == frame.localTeam)   continue;
            if (ent.health == 0)               continue;

            Vector3 pos{};
            float fov = 0.0f;
            if (!PickBestBone(ent, config.aim_hitbox, eyePos, viewAngles, pos, fov))
                continue;
            if (fov > bestFov) continue;

            bestFov = fov;
            best = &ent;
            bestPos = pos;
        }

        if (!best) return;

        const Vector3 desired = Math::CalcAngle(eyePos, bestPos);
        Vector3       finalAngle = Math::Smooth(viewAngles, desired, config.aim_smooth);
        finalAngle.z = 0.0f;
        finalAngle = Math::Clamp(finalAngle);

        if (!std::isfinite(finalAngle.x) || !std::isfinite(finalAngle.y)) return;

        mem.Write<Vector3>(
            mem.client + cs2_dumper::offsets::client_dll::dwViewAngles, finalAngle);
    }
}