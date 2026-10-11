#pragma once
#include "../Vectors/Vectors.hpp"
#include <cmath>
#include <algorithm>

class Math {
public:
    static constexpr float PI = 3.14159265358979323846f;

    static Vector3 CalcAngle(const Vector3& src, const Vector3& dst) {
        Vector3 delta = dst - src;
        Vector3 angles;
        angles.y = atan2f(delta.y, delta.x) * 180.f / PI;
        angles.x = -atan2f(delta.z, delta.Length2D()) * 180.f / PI;
        angles.z = 0.f;
        return angles;
    }

    static Vector3 Clamp(Vector3 a) {
        a.x = std::clamp(a.x, -89.f, 89.f);
        a.y = std::remainderf(a.y, 360.f);
        if (a.y > 180.f) a.y -= 360.f;
        if (a.y < -180.f) a.y += 360.f;
        a.z = 0.f;
        return a;
    }

    static float GetFov(const Vector3& viewAngles, const Vector3& aimAngles) {
        Vector3 delta = Clamp(aimAngles - viewAngles);
        return delta.Length2D();
    }

    static Vector3 Smooth(const Vector3& cur, const Vector3& target, float smooth) {
        if (smooth < 1.f) return target;
        Vector3 delta = Clamp(target - cur);
        return Clamp(cur + delta / smooth);
    }

    static bool WorldToScreen(const Vector3& world, Vector2& screen, const ViewMatrix& vm, int width, int height) {
        float w = vm[3][0] * world.x + vm[3][1] * world.y + vm[3][2] * world.z + vm[3][3];
        if (w < 0.01f)
            return false;

        float invW = 1.f / w;
        float x = (vm[0][0] * world.x + vm[0][1] * world.y + vm[0][2] * world.z + vm[0][3]) * invW;
        float y = (vm[1][0] * world.x + vm[1][1] * world.y + vm[1][2] * world.z + vm[1][3]) * invW;

        screen.x = (x + 1.f) * 0.5f * width;
        screen.y = (1.f - y) * 0.5f * height;
        return true;
    }

    static void AngleVectors(const Vector3& angles, Vector3& forward, Vector3& right, Vector3& up) {
        float sp = sinf(angles.x * PI / 180.f);
        float cp = cosf(angles.x * PI / 180.f);
        float sy = sinf(angles.y * PI / 180.f);
        float cy = cosf(angles.y * PI / 180.f);

        forward.x = cp * cy;
        forward.y = cp * sy;
        forward.z = -sp;

        right.x = -sy;
        right.y = cy;
        right.z = 0.f;

        up.x = sp * cy;
        up.y = sp * sy;
        up.z = cp;
    }
};