#pragma once
#include <cmath>

struct Vector2 {
    float x, y;
    Vector2(float x = 0, float y = 0) : x(x), y(y) {}
};

struct Vector3 {
    float x, y, z;
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }
    Vector3 operator/(float s) const { return { x / s, y / s, z / s }; }

    Vector3& operator+=(const Vector3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vector3& operator-=(const Vector3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    Vector3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

    bool operator==(const Vector3& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const Vector3& o) const { return !(*this == o); }

    float Dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float LengthSqr() const { return x * x + y * y + z * z; }
    float Length2D() const { return std::sqrt(x * x + y * y); }
    float DistTo(const Vector3& o) const { return (*this - o).Length(); }
    float DistToSqr(const Vector3& o) const { return (*this - o).LengthSqr(); }

    Vector3 Normalize() const {
        float l = Length();
        return l > 0.0001f ? *this * (1.f / l) : Vector3{ 0, 0, 0 };
    }
};

struct ViewMatrix {
    float m[4][4];
    const float* operator[](int index) const { return m[index]; }
    float* operator[](int index) { return m[index]; }
};