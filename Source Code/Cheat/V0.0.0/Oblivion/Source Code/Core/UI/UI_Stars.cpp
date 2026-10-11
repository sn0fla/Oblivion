#include "UI_Stars.hpp"
#include "UI_Internal.hpp"
#include <cmath>

namespace UI {
    namespace Stars {

        using namespace Internal;

        void Draw(ImDrawList* dl, ImVec2 min, ImVec2 max, float time, float alpha)
        {
            if (alpha < 0.02f) return;
            if (max.x <= min.x || max.y <= min.y) return;

            const float w = max.x - min.x;
            const float h = max.y - min.y;

            const int kLayers = 5;
            const int kPerLayer[5] = { 520, 340, 210, 110, 48 };

            const float layerAlpha[5] = { 0.32f, 0.55f, 0.78f, 0.92f, 1.00f };
            const float sizeMin[5] = { 0.18f, 0.28f, 0.44f, 0.64f, 0.88f };
            const float sizeMax[5] = { 0.40f, 0.58f, 0.85f, 1.15f, 1.55f };
            const float speedMin[5] = { 0.20f, 0.34f, 0.52f, 0.72f, 0.95f };
            const float speedMax[5] = { 0.75f, 1.05f, 1.45f, 1.90f, 2.40f };

            for (int layer = 0; layer < kLayers; ++layer) {
                const int n = kPerLayer[layer];
                for (int i = 0; i < n; ++i) {
                    const unsigned int seed =
                        (unsigned int)(layer * 2371 + i * 53 + 11);

                    const float rx = Hash01(seed * 3u + 1u);
                    const float ry = Hash01(seed * 5u + 7u);
                    const float rp = Hash01(seed * 7u + 11u);
                    const float rs = Hash01(seed * 11u + 13u);
                    const float rv = Hash01(seed * 13u + 17u);
                    const float rc = Hash01(seed * 17u + 19u);
                    const float rsq = Hash01(seed * 19u + 23u);

                    const float px = min.x + rx * w;
                    const float py = min.y + ry * h;

                    const float base = sizeMin[layer] + rs * (sizeMax[layer] - sizeMin[layer]);
                    const float size = base * (0.80f + rsq * 0.40f);
                    const float spd = speedMin[layer] + rv * (speedMax[layer] - speedMin[layer]);

                    float tw = 0.5f + 0.5f * sinf(time * spd + rp * 6.2831853f);
                    tw = tw * tw * tw;
                    tw = tw * tw;

                    const float starA = alpha * layerAlpha[layer] * (0.08f + 0.92f * tw);
                    int a8 = (int)(starA * 255.0f);
                    if (a8 < 3) continue;
                    if (a8 > 255) a8 = 255;

                    int rTint = 255;
                    int gTint = 255;
                    int bTint = 255;

                    if (rc < 0.20f) {
                        rTint = 205; gTint = 218; bTint = 255;
                    }
                    else if (rc < 0.34f) {
                        rTint = 255; gTint = 240; bTint = 215;
                    }
                    else if (rc < 0.45f) {
                        rTint = 235; gTint = 225; bTint = 255;
                    }

                    const ImVec2 p(px, py);

                    if (layer >= 3) {
                        const int haloA = (int)(a8 * 0.14f);
                        if (haloA > 2) {
                            dl->AddCircleFilled(p, size * 3.2f,
                                IM_COL32(rTint, gTint, bTint, haloA), 12);
                        }

                        const int midA = (int)(a8 * 0.38f);
                        if (midA > 2) {
                            dl->AddCircleFilled(p, size * 1.65f,
                                IM_COL32(rTint, gTint, bTint, midA), 12);
                        }
                    }
                    else if (layer == 2) {
                        const int midA = (int)(a8 * 0.28f);
                        if (midA > 2) {
                            dl->AddCircleFilled(p, size * 1.55f,
                                IM_COL32(rTint, gTint, bTint, midA), 10);
                        }
                    }

                    dl->AddCircleFilled(p, size,
                        IM_COL32(rTint, gTint, bTint, a8), 12);

                    if (layer == 4 && a8 > 140) {
                        const int spikeA = (int)(a8 * 0.45f);
                        const float sLen = size * 3.0f;
                        const float sTh = 0.75f;
                        const ImU32 sc = IM_COL32(rTint, gTint, bTint, spikeA);

                        dl->AddLine(ImVec2(px - sLen, py), ImVec2(px + sLen, py), sc, sTh);
                        dl->AddLine(ImVec2(px, py - sLen), ImVec2(px, py + sLen), sc, sTh);
                    }
                }
            }
        }

    }
}