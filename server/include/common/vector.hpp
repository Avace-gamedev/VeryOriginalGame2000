#pragma once

#include <math.h>

class Vec2i
{
public:
    int x;
    int y;

    Vec2i(int x, int y) : x(x), y(y){};
    Vec2i() : Vec2i(0, 0){};

    int sqrNorm()
    {
        return (this->x * this->x) + (this->y * this->y);
    }

    float norm()
    {
        return (float)sqrt(this->sqrNorm());
    }

    Vec2i &operator+=(const Vec2i &other)
    {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }
    Vec2i &operator-=(const Vec2i &other)
    {
        this->x -= other.x;
        this->y -= other.y;
        return *this;
    }
    Vec2i &operator*=(const int other)
    {
        this->x *= other;
        this->y *= other;
        return *this;
    }
};

inline Vec2i operator+(const Vec2i a, const Vec2i b)
{
    return Vec2i(a.x + b.x, a.y + b.y);
}

inline Vec2i operator-(const Vec2i a, const Vec2i b)
{
    return Vec2i(a.x - b.x, a.y - b.y);
}

inline Vec2i operator*(const Vec2i a, const int b)
{
    return Vec2i(a.x * b, a.y * b);
}

inline Vec2i operator*(const int a, const Vec2i b)
{
    return Vec2i(a * b.x, a * b.y);
}

inline Vec2i operator/(const Vec2i a, const int b)
{
    return Vec2i(a.x / b, a.y / b);
}

inline Vec2i operator/(const int a, const Vec2i b)
{
    return Vec2i(a / b.x, a / b.y);
}

// Vec2f

class Vec2f
{
public:
    float x;
    float y;

    Vec2f(float x, float y) : x(x), y(y){};
    Vec2f() : Vec2f(0, 0){};

    float sqrNorm()
    {
        return (this->x * this->x) + (this->y * this->y);
    }

    float norm()
    {
        return (float)sqrt(this->sqrNorm());
    }

    Vec2f &operator+=(const Vec2f &other)
    {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }
    Vec2f &operator-=(const Vec2f &other)
    {
        this->x -= other.x;
        this->y -= other.y;
        return *this;
    }
    Vec2f &operator*=(const float other)
    {
        this->x *= other;
        this->y *= other;
        return *this;
    }

    const bool operator==(const Vec2f &other)
    {
        return (this->x == other.x) && (this->y == other.y);
    }

    const bool operator!=(const Vec2f &other)
    {
        return (this->x != other.x) || (this->y != other.y);
    }
};

inline Vec2f operator+(const Vec2f a, const Vec2f b)
{
    return Vec2f(a.x + b.x, a.y + b.y);
}

inline Vec2f operator-(const Vec2f a, const Vec2f b)
{
    return Vec2f(a.x - b.x, a.y - b.y);
}

inline float operator*(const Vec2f a, const Vec2f b)
{
    return a.x * b.x + a.y * b.y;
}

inline Vec2f operator*(const Vec2f a, const float b)
{
    return Vec2f(a.x * b, a.y * b);
}

inline Vec2f operator*(const float a, const Vec2f b)
{
    return Vec2f(a * b.x, a * b.y);
}

inline Vec2f operator/(const Vec2f a, const float b)
{
    return Vec2f(a.x / b, a.y / b);
}

inline Vec2f operator/(const float a, const Vec2f b)
{
    return Vec2f(a / b.x, a / b.y);
}