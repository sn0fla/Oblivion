#include "Skeleton.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include "../../../../../../Utilities/Config/Config.hpp"
#include "../../../../../../External/ImGUI/imgui.h"
#include <algorithm>

namespace Features::ESP {

    struct BoneConnection { BoneIndex a; BoneIndex b; };

    static const BoneConnection kConnections[] = {
        { BoneIndex::Head,          BoneIndex::Neck },
        { BoneIndex::Neck,          BoneIndex::Spine_2 },
        { BoneIndex::Spine_2,       BoneIndex::Spine_1 },
        { BoneIndex::Spine_1,       BoneIndex::Spine_0 },
        { BoneIndex::Spine_0,       BoneIndex::Pelvis },
        { BoneIndex::Spine_2,       BoneIndex::LeftClavicle },
        { BoneIndex::LeftClavicle,  BoneIndex::LeftUpperArm },
        { BoneIndex::LeftUpperArm,  BoneIndex::LeftForearm },
        { BoneIndex::LeftForearm,   BoneIndex::LeftHand },
        { BoneIndex::Spine_2,       BoneIndex::RightClavicle },
        { BoneIndex::RightClavicle, BoneIndex::RightUpperArm },
        { BoneIndex::RightUpperArm, BoneIndex::RightForearm },
        { BoneIndex::RightForearm,  BoneIndex::RightHand },
        { BoneIndex::Pelvis,        BoneIndex::LeftHip },
        { BoneIndex::LeftHip,       BoneIndex::LeftKnee },
        { BoneIndex::LeftKnee,      BoneIndex::LeftFoot },
        { BoneIndex::Pelvis,        BoneIndex::RightHip },
        { BoneIndex::RightHip,      BoneIndex::RightKnee },
        { BoneIndex::RightKnee,     BoneIndex::RightFoot },
    };
    static constexpr int kConnectionCount = sizeof(kConnections) / sizeof(kConnections[0]);

    static constexpr uint8_t kConnGroup[19] = {
        0,
        1, 1, 1, 1,
        2, 2, 2, 2,
        3, 3, 3, 3,
        4, 4,
        6,
        5, 5,
        6
    };

    static inline bool ShouldDrawConnection(int idx, uint32_t mask) {
        return ((mask >> kConnGroup[idx]) & 1u) != 0u;
    }

    static inline ImU32 Col(const float c[4]) {
        return IM_COL32(static_cast<int>(c[0] * 255.0f),
            static_cast<int>(c[1] * 255.0f),
            static_cast<int>(c[2] * 255.0f),
            static_cast<int>(c[3] * 255.0f));
    }

    static void RenderSide(const FrameSnapshot& frame, const ViewMatrix& viewMatrix,
        int width, int height, bool isEnemy)
    {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        const float* colArr = isEnemy ? config.esp_enemy_skeleton_color
            : config.esp_team_skeleton_color;
        const uint32_t jointsMask = isEnemy ? config.esp_enemy_skeleton_joints
            : config.esp_team_skeleton_joints;
        const bool drawJoints = isEnemy ? config.esp_enemy_skeleton_draw_joints
            : config.esp_team_skeleton_draw_joints;
        const ImU32 col = Col(colArr);
        constexpr float kThickness = 1.5f;

        if (jointsMask == 0 && !drawJoints) return;

        const float jointRadius = (std::max)(2.0f, height * 0.0028f);

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;
            if (!ent.hasBones) continue;

            Vector2 scr[kBoneCount];
            bool valid[kBoneCount] = {};

            for (int i = 0; i < kBoneCount; ++i) {
                if (Math::WorldToScreen(ent.bones[i], scr[i], viewMatrix, width, height))
                    valid[i] = true;
            }

            bool drawn[kBoneCount] = {};

            for (int c = 0; c < kConnectionCount; ++c) {
                if (!ShouldDrawConnection(c, jointsMask)) continue;
                const int a = static_cast<int>(kConnections[c].a);
                const int b = static_cast<int>(kConnections[c].b);
                if (a >= kBoneCount || b >= kBoneCount) continue;

                if (valid[a] && valid[b]) {
                    dl->AddLine(ImVec2(scr[a].x, scr[a].y),
                        ImVec2(scr[b].x, scr[b].y), col, kThickness);
                }

                if (drawJoints) {
                    if (valid[a] && !drawn[a]) {
                        dl->AddCircleFilled(ImVec2(scr[a].x, scr[a].y),
                            jointRadius, col, 10);
                        drawn[a] = true;
                    }
                    if (valid[b] && !drawn[b]) {
                        dl->AddCircleFilled(ImVec2(scr[b].x, scr[b].y),
                            jointRadius, col, 10);
                        drawn[b] = true;
                    }
                }
            }
        }
    }

    void RenderSkeletonESP(const FrameSnapshot& frame, const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_skeleton)
            RenderSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_skeleton)
            RenderSide(frame, viewMatrix, width, height, false);
    }
}