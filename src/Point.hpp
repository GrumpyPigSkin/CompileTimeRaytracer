#ifndef POINT_H_
#define POINT_H_

#include <cmath>

struct Point {
  float x{};
  float y{};
  float z{};

  constexpr friend bool operator<=>(const Point &lhs,
                                    const Point &rhs) = default;

  constexpr friend auto operator+(Point lhs, const Point rhs) -> Point {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;
  }

  constexpr friend auto operator-(Point lhs, const Point rhs) -> Point {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    return lhs;
  }

  constexpr friend auto operator*(Point lhs, const Point rhs) -> Point {
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    lhs.z *= rhs.z;
    return lhs;
  }

  constexpr friend auto operator*(Point lhs, const float rhs) -> Point {
    lhs.x *= rhs;
    lhs.y *= rhs;
    lhs.z *= rhs;
    return lhs;
  }

  constexpr friend auto operator*(const float rhs, Point lhs) -> Point {
    lhs.x *= rhs;
    lhs.y *= rhs;
    lhs.z *= rhs;
    return lhs;
  }

  constexpr friend auto operator/(Point lhs, const Point rhs) -> Point {
    lhs.x /= rhs.x;
    lhs.y /= rhs.y;
    lhs.z /= rhs.z;
    return lhs;
  }

  constexpr friend auto operator/(Point lhs, const float rhs) -> Point {
    lhs.x /= rhs;
    lhs.y /= rhs;
    lhs.z /= rhs;
    return lhs;
  }

  constexpr friend auto pow(Point point, float exp) -> Point {
    point.x = std::pow(point.x, exp);
    point.y = std::pow(point.y, exp);
    point.z = std::pow(point.z, exp);
    return point;
  }

  constexpr friend auto sqrt(Point point) -> Point {
    point.x = std::sqrt(point.x);
    point.y = std::sqrt(point.y);
    point.z = std::sqrt(point.z);
    return point;
  }

  constexpr friend auto sum(const Point a) -> float { return a.x + a.y + a.z; }

  constexpr friend auto DotProduct(const Point a, const Point b) -> float {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }

  constexpr friend auto LengthSquared(const Point a) -> float {
    return a.x * a.x + a.y * a.y + a.z * a.z;
  }

  constexpr friend auto Length(const Point point) -> float {
    return std::sqrt(LengthSquared(point));
  }

  constexpr friend auto Normalize(const Point point) -> Point {
    return point / Length(point);
  }

  constexpr friend auto CrossProduct(const Point a, const Point b) -> Point {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
  }
};

#endif
