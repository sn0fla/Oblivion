#pragma once
#include "../../../../../../SDK/Entities/Entities.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include <d3d11.h>
#include <cmath>
#include <algorithm>

namespace Features::ESP {
    struct BoxRect {
        bool valid = false;
        float left = 0.0f, top = 0.0f, right = 0.0f, bottom = 0.0f;
        float centerX = 0.0f;
        float width() const { return right - left; }
        float height() const { return bottom - top; }
    };

    inline BoxRect ComputeBox(const FrameEntity& ent, const ViewMatrix& vm, int width, int height) {
        BoxRect rect;
        if (ent.boxCached) {
            rect.valid = ent.boxValid;
            rect.left = ent.boxL;
            rect.top = ent.boxT;
            rect.right = ent.boxR;
            rect.bottom = ent.boxB;
            rect.centerX = ent.boxCx;
            return rect;
        }

        do {
            Vector2 feetScreen, headScreen, centerScreen;
            if (!Math::WorldToScreen(ent.feetPos, feetScreen, vm, width, height)) break;
            if (!Math::WorldToScreen(ent.headPos, headScreen, vm, width, height)) break;

            const Vector3 centerWorld = ent.hasBones
                ? ent.bones[static_cast<int>(BoneIndex::Spine_1)]
                : ent.origin;

            const bool centerOk = Math::WorldToScreen(centerWorld, centerScreen, vm, width, height);

            if (!std::isfinite(feetScreen.y) || !std::isfinite(headScreen.y)) break;

            const float centerX = centerOk ? centerScreen.x : feetScreen.x;
            if (!std::isfinite(centerX)) break;

            const float rawH = feetScreen.y - headScreen.y;
            if (rawH < 1.0f) break;

            const float fh = static_cast<float>(height);
            const float closeT = (std::clamp)(rawH / (fh * 0.75f), 0.0f, 1.0f);

            const float kTopMul = 0.15f + closeT * 0.20f;
            const float kBotMul = 0.15f + closeT * 0.10f;
            const float kAspect = 0.60f + closeT * 0.25f;

            const float boxH = rawH * (1.0f + kTopMul + kBotMul);
            const float boxW = boxH * kAspect;

            if (boxH > fh * 1.5f || boxW > static_cast<float>(width) * 1.5f) break;

            const float topY = headScreen.y - rawH * kTopMul;
            const float botY = topY + boxH;
            const float left = centerX - (boxW * 0.5f);
            const float right = left + boxW;

            rect.valid = true;
            rect.left = left;
            rect.top = topY;
            rect.right = right;
            rect.bottom = botY;
            rect.centerX = centerX;
        } while (false);

        ent.boxCached = true;
        ent.boxValid = rect.valid;
        ent.boxL = rect.left;
        ent.boxT = rect.top;
        ent.boxR = rect.right;
        ent.boxB = rect.bottom;
        ent.boxCx = rect.centerX;
        return rect;
    }

    void RenderBoxESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        int width, int height);
}