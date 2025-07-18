#ifndef COLOUR_H_
#define COLOUR_H_

struct Colour {
  float r{};
  float g{};
  float b{};

  constexpr friend bool operator<=>(const Colour &lhs,
                                    const Colour &rhs) = default;

  constexpr friend auto operator+(Colour lhs, const Colour rhs) -> Colour {
    lhs.r += rhs.r;
    lhs.g += rhs.g;
    lhs.b += rhs.b;
    return lhs;
  }

  constexpr friend auto operator-(Colour lhs, const Colour rhs) -> Colour {
    lhs.r -= rhs.r;
    lhs.g -= rhs.g;
    lhs.b -= rhs.b;
    return lhs;
  }

  constexpr friend auto operator*(Colour lhs, const Colour rhs) -> Colour {
    lhs.r *= rhs.r;
    lhs.g *= rhs.g;
    lhs.b *= rhs.b;
    return lhs;
  }

  constexpr friend auto operator*(Colour lhs, const float rhs) -> Colour {
    lhs.r *= rhs;
    lhs.g *= rhs;
    lhs.b *= rhs;
    return lhs;
  }

  constexpr friend auto operator*(const float rhs, Colour lhs) -> Colour {
    return lhs * rhs;
  }

  constexpr friend auto operator/(Colour lhs, const Colour rhs) -> Colour {
    lhs.r /= rhs.r;
    lhs.g /= rhs.g;
    lhs.b /= rhs.b;
    return lhs;
  }

  constexpr friend auto operator/(Colour lhs, const float rhs) -> Colour {
    lhs.r /= rhs;
    lhs.g /= rhs;
    lhs.b /= rhs;
    return lhs;
  }
};

#endif
