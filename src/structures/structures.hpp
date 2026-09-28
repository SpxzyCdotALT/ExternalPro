#pragma once

#include <cmath>

struct Vector3 final {
    float x, y, z;

    Vector3() noexcept;
    Vector3(float x, float y, float z) noexcept;

    const float& operator[](int i) const noexcept;
    float& operator[](int i) noexcept;

    Vector3 operator/(float s) const noexcept;
    float dot(const Vector3& vec) const noexcept;
    float distance(const Vector3& vector) const noexcept;
    Vector3 operator*(float value) const noexcept;
    bool operator!=(const Vector3& other) const noexcept;
    float squared() const noexcept;
    Vector3 normalize() const noexcept;
    Vector3 direction() const noexcept;

    static const Vector3& one() noexcept;
    static const Vector3& unitX() noexcept;
    static const Vector3& unitY() noexcept;
    static const Vector3& unitZ() noexcept;

    Vector3 cross(const Vector3& b) const noexcept;
    Vector3 operator+(const Vector3& vec) const noexcept;
    Vector3 operator-(const Vector3& vec) const noexcept;
    Vector3 operator*(const Vector3& vec) const noexcept;
    Vector3 operator/(const Vector3& vec) const noexcept;

    Vector3& operator+=(const Vector3& vec) noexcept;
    Vector3& operator-=(const Vector3& vec) noexcept;
    Vector3& operator*=(float fScalar) noexcept;
    Vector3& operator/=(const Vector3& other) noexcept;

    bool operator==(const Vector3& other) const noexcept;
    float magnitude() const noexcept;
};

struct Vector2 final {
    float x, y;

    Vector2 operator-(const Vector2& other) const noexcept;
    Vector2 operator+(const Vector2& other) const noexcept;
    Vector2 operator/(float factor) const noexcept;
    Vector2 operator/(const Vector2& factor) const noexcept;
    Vector2 operator*(float factor) const noexcept;
    Vector2 operator*(const Vector2& factor) const noexcept;

    float getMagnitude() const noexcept;
};

struct Matrix4 final {
    float data[16];
};