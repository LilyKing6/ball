#ifndef MATH_H
#define MATH_H

#include <QtCore>
#include <QtMath>

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float length() const { return qSqrt(x * x + y * y); }
    float lengthSq() const { return x * x + y * y; }
    Vec2 normalized() const { float l = length(); return l > 0 ? *this / l : Vec2{}; }
    void normalize() { float l = length(); if (l > 0) { x /= l; y /= l; } }

    static float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
    static float cross(const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; }
    static Vec2 lerp(const Vec2& a, const Vec2& b, float t) { return a + (b - a) * t; }
};

inline float easeOutCubic(float t) { return 1.0f - qPow(1.0f - t, 3.0f); }
inline float clamp(float v, float min, float max) { return qBound(min, v, max); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

#endif // MATH_H
