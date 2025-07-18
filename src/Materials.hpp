#ifndef MATERIALS_H_
#define MATERIALS_H_

#include "Colour.hpp"
#include <variant>

struct Lambertian {
  Colour albedo{};
};

struct Metal {
  Colour albedo{};
  float fuzz{};
};

struct Dielectric {
  float refractiveIndex{};
};

struct Emissive {
  Colour albedo;
};

using MaterialT = std::variant<Lambertian, Metal, Dielectric, Emissive>;

#endif
