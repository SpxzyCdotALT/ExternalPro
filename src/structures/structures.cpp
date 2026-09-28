#include "structures.hpp"

Vector3::Vector3() noexcept : x(0), y(0), z(0) {}

Vector3::Vector3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

const float& Vector3::operator[](int i) const noexcept {
    return reinterpret_cast<const float*>(this)[i];
}

float& Vector3::operator[](int i) noexcept {
    return reinterpret_cast<float*>(this)[i];
}

Vector3 Vector3::operator/(float s) const noexcept {
    return *this * (1.0f / s);
}

float Vector3::dot(const Vector3& vec) const noexcept {
    return x * vec.x + y * vec.y + z * vec.z;
}

float Vector3::distance(const Vector3& vector) const noexcept {
    return std::sqrt((vector.x - x) * (vector.x - x) +
        (vector.y - y) * (vector.y - y) +
        (vector.z - z) * (vector.z - z));
}

Vector3 Vector3::operator*(float value) const noexcept {
    return { x * value, y * value, z * value };
}

bool Vector3::operator!=(const Vector3& other) const noexcept {
    return x != other.x || y != other.y || z != other.z;
}

float Vector3::squared() const noexcept {
    return x * x + y * y + z * z;
}

Vector3 Vector3::normalize() const noexcept {
    float mag = magnitude();
    if (mag == 0) return { 0, 0, 0 };
    return { x / mag, y / mag, z / mag };
}

Vector3 Vector3::direction() const noexcept {
    return normalize();
}

const Vector3& Vector3::one() noexcept {
    static const Vector3 v(1, 1, 1);
    return v;
}

const Vector3& Vector3::unitX() noexcept {
    static const Vector3 v(1, 0, 0);
    return v;
}

const Vector3& Vector3::unitY() noexcept {
    static const Vector3 v(0, 1, 0);
    return v;
}

const Vector3& Vector3::unitZ() noexcept {
    static const Vector3 v(0, 0, 1);
    return v;
}

Vector3 Vector3::cross(const Vector3& b) const noexcept {
    return { y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x };
}

Vector3 Vector3::operator+(const Vector3& vec) const noexcept {
    return { x + vec.x, y + vec.y, z + vec.z };
}

Vector3 Vector3::operator-(const Vector3& vec) const noexcept {
    return { x - vec.x, y - vec.y, z - vec.z };
}

Vector3 Vector3::operator*(const Vector3& vec) const noexcept {
    return { x * vec.x, y * vec.y, z * vec.z };
}

Vector3 Vector3::operator/(const Vector3& vec) const noexcept {
    return { x / vec.x, y / vec.y, z / vec.z };
}

Vector3& Vector3::operator+=(const Vector3& vec) noexcept {
    x += vec.x;
    y += vec.y;
    z += vec.z;
    return *this;
}

Vector3& Vector3::operator-=(const Vector3& vec) noexcept {
    x -= vec.x;
    y -= vec.y;
    z -= vec.z;
    return *this;
}

Vector3& Vector3::operator*=(float fScalar) noexcept {
    x *= fScalar;
    y *= fScalar;
    z *= fScalar;
    return *this;
}

Vector3& Vector3::operator/=(const Vector3& other) noexcept {
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}

bool Vector3::operator==(const Vector3& other) const noexcept {
    return x == other.x && y == other.y && z == other.z;
}

float Vector3::magnitude() const noexcept {
    return std::sqrt(x * x + y * y + z * z);
}

Vector2 Vector2::operator-(const Vector2& other) const noexcept {
    return { x - other.x, y - other.y };
}

Vector2 Vector2::operator+(const Vector2& other) const noexcept {
    return { x + other.x, y + other.y };
}

Vector2 Vector2::operator/(float factor) const noexcept {
    return { x / factor, y / factor };
}

Vector2 Vector2::operator/(const Vector2& factor) const noexcept {
    return { x / factor.x, y / factor.y };
}

Vector2 Vector2::operator*(float factor) const noexcept {
    return { x * factor, y * factor };
}

Vector2 Vector2::operator*(const Vector2& factor) const noexcept {
    return { x * factor.x, y * factor.y };
}

float Vector2::getMagnitude() const noexcept {
    return std::sqrt(x * x + y * y);
}