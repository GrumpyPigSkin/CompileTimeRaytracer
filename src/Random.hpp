#ifndef COMPILE_TIME_RANDOM_H_
#define COMPILE_TIME_RANDOM_H_

#include "Point.hpp"
#include <cstdint>
#include <limits>
#include <string_view>

struct CompileTimeRandom {
  std::uint64_t state;
  constexpr auto operator()() -> std::uint64_t {
    std::uint64_t x = state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return state = x;
  }
  constexpr auto random_float() -> float {
    return static_cast<float>(operator()()) /
           static_cast<float>(std::numeric_limits<uint64_t>::max());
  }
};

constexpr auto CreateSeed() {
  constexpr auto time_str = std::string_view(__TIME__);
  std::uint64_t seed = 0;
  for (char c : time_str) {
    if (c >= '0' && c <= '9') {
      seed = seed * 10 + (c - '0');
    }
  }
  return seed;
}

constexpr auto RandomPointInUnitSphere(CompileTimeRandom &rng) -> Point {
  float x{}, y{}, z{}, length_squared{};
  do {
    x = 2.f * rng.random_float() - 1.f;
    y = 2.f * rng.random_float() - 1.f;
    z = 2.f * rng.random_float() - 1.f;
    length_squared = x * x + y * y + z * z;
  } while (length_squared >= 1.f);
  return {x, y, z};
}

#endif
