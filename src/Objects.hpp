#ifndef OBJECTS_H_
#define OBJECTS_H_

#include "Materials.hpp"
#include "Point.hpp"
#include <array>
#include <optional>

struct Ray {
  constexpr Ray() = default;
  constexpr Ray(Point origin, Point direction)
      : origin(origin), direction(Normalize(direction)) {}

  Point origin;
  Point direction;
};

struct Sphere {
  MaterialT mat;
  Point center;
  float radius{};
};

struct Rectangle {
  MaterialT mat;
  Point origin;
  Point u;
  Point v;
  Point normal{};
  float u_len_sq{};
  float v_len_sq{};
};

using ObjectT = std::variant<Sphere, Rectangle>;

template <std::size_t NUM_OBJECTS> struct Environs {
  std::array<ObjectT, NUM_OBJECTS> objects;
};

struct HitRecord {
  MaterialT mat;
  Point point;
  Point normal;
  float t{};
};

constexpr auto HitRectangle(const Rectangle &rect, const Ray &r,
                            const float tmin, const float tmax)
    -> std::optional<HitRecord> {

  const float denom = DotProduct(r.direction, rect.normal);
  if (std::abs(denom) < 1e-6) {
    return std::nullopt;
  }

  const float t = DotProduct(rect.origin - r.origin, rect.normal) / denom;

  if (t < tmin || t > tmax) {
    return std::nullopt;
  }

  const Point hit_point = r.origin + t * r.direction;
  const Point hit_vec = hit_point - rect.origin;

  const float u_proj = DotProduct(hit_vec, rect.u);
  const float v_proj = DotProduct(hit_vec, rect.v);

  if (u_proj >= 0 && u_proj <= rect.u_len_sq && v_proj >= 0 &&
      v_proj <= rect.v_len_sq) {

    HitRecord rec;
    rec.t = t;
    rec.point = hit_point;
    rec.mat = rect.mat;
    rec.normal = (denom < 0) ? rect.normal : Point{} - rect.normal;
    return rec;
  }

  return std::nullopt;
}

constexpr auto HitSphere(const Sphere &s, const Ray &r, const float tmin,
                         const float tmax) -> std::optional<HitRecord> {
  Point oc = r.origin - s.center;
  float a = DotProduct(r.direction, r.direction);
  float b = 2 * DotProduct(r.direction, oc);
  float c = DotProduct(oc, oc) - std::pow(s.radius, 2.f);
  float disc = std::pow(b, 2.f) - 4.f * a * c;
  float t{};

  auto try_hit = [&](float t) -> std::optional<HitRecord> {
    if (t > tmin && t < tmax) {
      HitRecord rec;
      rec.t = t;
      rec.point = r.origin + r.direction * t;
      rec.normal = (rec.point - s.center) / s.radius;
      rec.mat = s.mat;
      return rec;
    }
    return std::nullopt;
  };

  if (disc >= 0.f) {
    float sqrtd = std::sqrt(disc);
    if (auto hit = try_hit((-b - sqrtd) / (2.f * a))) {
      return hit;
    }
    if (auto hit = try_hit((-b + sqrtd) / (2.f * a))) {
      return hit;
    }
  }
  return std::nullopt;
}

constexpr auto HitEnvirons(const auto &environs, const Ray &r, float tmin,
                           float tmax) -> std::optional<HitRecord> {
  float closest = tmax;
  std::optional<HitRecord> res = std::nullopt;

  auto overloads =
      Overload([&](const Sphere &s) { return HitSphere(s, r, tmin, closest); },
               [&](const Rectangle &rect) {
                 return HitRectangle(rect, r, tmin, closest);
               });

  for (const auto &env : environs.objects) {
    if (auto hit = std::visit(overloads, env)) {
      closest = hit->t;
      res = hit;
    }
  }

  return res;
}

#endif
