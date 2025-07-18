#include "Colour.hpp"
#include "Materials.hpp"
#include "Objects.hpp"
#include "Overloaded.hpp"
#include "Point.hpp"
#include "Random.hpp"
#include "mdspan.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fmt/format.h>
#include <fmt/os.h>
#include <fmt/xchar.h>
#include <limits>
#include <optional>
#include <pthread.h>
#include <variant>

namespace {

constexpr auto SIZE_X = 100;
constexpr auto SIZE_Y = 50;
constexpr auto MAX_DEPTH = 10;
constexpr auto NUMBER_OF_RAYS_PER_PIXEL = 20;

using SkySpan = std::mdspan<Colour, std::extents<size_t, SIZE_Y, SIZE_X>>;

struct Skybox {
  SkySpan texture;
};

struct Camera {
  Point lowerleftCorner;
  Point horizontal;
  Point vertical;
  Point origin;
};

constexpr auto Reflect(const Ray rin, const Point n) -> Point {
  return rin.direction - 2.f * DotProduct(rin.direction, n) * n;
}

constexpr auto Refract(const Ray rin, const Point n, const float niOverDt)
    -> std::optional<Point> {
  const auto rporj = DotProduct(rin.direction, n);
  const auto costT2 = 1.f - (niOverDt * niOverDt) * (1 - (rporj * rporj));
  if (costT2 > 0.f) {
    return niOverDt * (rin.direction - rporj * n) - std::sqrt(costT2) * n;
  }
  return std::nullopt;
}

constexpr auto Schlick(const float cosine, const float refractiveIndex)
    -> float {
  const float res =
      std::pow(((1.f - refractiveIndex) / (1.f + refractiveIndex)), 2.f);
  return res + (1.f - res) * std::pow((1.f - cosine), 5.f);
}

constexpr Colour Emit(const MaterialT &mat) {
  auto overloads = Overload([](const Emissive &m) { return m.albedo; },
                            [](const auto &) { return Colour{0.f, 0.f, 0.f}; });
  return std::visit(overloads, mat);
}

constexpr auto Scatter(Ray rin, HitRecord rec, CompileTimeRandom &rng,
                       Colour &attenuation, Ray &rscattered) -> bool {

  const auto visit_lambertian = [&](const Lambertian mat) {
    const auto n = Normalize(rec.normal);
    const auto dir = n + RandomPointInUnitSphere(rng);
    rscattered = Ray(rec.point, dir);
    attenuation = mat.albedo;
    return true;
  };

  const auto visit_metal = [&](const Metal mat) {
    Point n = Normalize(rec.normal);
    Point reflected = Reflect(rin, n);
    Point dir = reflected + mat.fuzz * RandomPointInUnitSphere(rng);
    rscattered = Ray(rec.point, dir);
    attenuation = mat.albedo;
    return DotProduct(rscattered.direction, n) > 0;
  };

  const auto visit_dielectric = [&](const Dielectric mat) {
    attenuation = Colour(1.f, 1.f, 1.f);
    Point n;
    float ni_over_nt{};
    float cosine{};
    float reflect_prob{};

    if (DotProduct(rin.direction, rec.normal) > 0) {
      n = Point{} - rec.normal;
      ni_over_nt = mat.refractiveIndex;
      cosine = ni_over_nt * DotProduct(rin.direction, rec.normal) /
               Length(rin.direction);
    } else {
      n = rec.normal;
      ni_over_nt = 1.f / mat.refractiveIndex;
      cosine = -DotProduct(rin.direction, rec.normal) / Length(rin.direction);
    }
    Point refracted;
    if (auto ref = Refract(rin, n, ni_over_nt)) {
      refracted = ref.value();
      reflect_prob = Schlick(cosine, mat.refractiveIndex);
    } else {
      reflect_prob = 1.f;
    }

    if (rng.random_float() < reflect_prob) {
      Point reflected = Reflect(rin, rec.normal);
      rscattered = Ray(rec.point, reflected);
    } else {
      rscattered = Ray(rec.point, refracted);
    }
    return true;
  };

  const auto visit_emissive = [&](const Emissive & /*mat*/) { return false; };

  const auto overloads =
      Overload(visit_lambertian, visit_metal, visit_dielectric, visit_emissive);

  return std::visit(overloads, rec.mat);
}

constexpr auto GetColour(const auto &environs, const Skybox &skybox, Ray r,
                         CompileTimeRandom &rng) -> Colour {
  constexpr auto MAX_DEPTH = 50;
  constexpr auto SHADOW_ACHE = .001f;
  constexpr auto HUGE_T = std::numeric_limits<float>::max();

  Colour attenuation{1.f, 1.f, 1.f};
  Colour final_colour{0.f, 0.f, 0.f};

  auto lray = r;
  for (int i = 0; i < MAX_DEPTH; i++) {
    if (auto rec = HitEnvirons(environs, lray, SHADOW_ACHE, HUGE_T)) {

      Ray sc_ray{};
      Colour sc_atten{};
      final_colour = final_colour + (attenuation * Emit(rec->mat));

      if (Scatter(lray, *rec, rng, sc_atten, sc_ray)) {
        attenuation = sc_atten * attenuation;
        lray = sc_ray;
      } else {
        break;
      }
    } else {
      Point unit_direction = Normalize(r.direction);

      constexpr float pi = std::numbers::pi_v<float>;
      float u =
          0.5f + std::atan2(unit_direction.z, unit_direction.x) / (2.f * pi);
      float v = 0.5f - std::asin(unit_direction.y) / pi;

      auto width = skybox.texture.extent(1);
      auto height = skybox.texture.extent(0);
      int x = static_cast<int>(u * (width - 1));
      int y = static_cast<int>(v * (height - 1));

      final_colour = final_colour + (attenuation * skybox.texture[y, x]);
      break;
    }
  }

  return final_colour;
}

constexpr auto Render(auto outputBuffer, const Skybox &skybox,
                      const auto &environs, const Camera &cam) -> void {
  Ray r;
  int ir{};
  constexpr int nx = SIZE_X;
  constexpr int ny = SIZE_Y;

  CompileTimeRandom rng{CreateSeed()};

  for (std::size_t i = 0; i != nx; i++) {
    for (std::size_t j = 0; j != ny; j++) {
      Colour c{};
      for (int k = 0; k < NUMBER_OF_RAYS_PER_PIXEL; k++) {
        const auto u = (static_cast<float>(i) + rng.random_float()) /
                       static_cast<float>(nx);
        const auto v = (static_cast<float>(j) + rng.random_float()) /
                       static_cast<float>(ny);
        const Ray r{cam.origin, cam.lowerleftCorner + u * cam.horizontal +
                                    v * cam.vertical - cam.origin};
        c = c + GetColour(environs, skybox, r, rng);
      }
      outputBuffer[i, j] = c / NUMBER_OF_RAYS_PER_PIXEL;
    }
  }
}

constexpr Rectangle CreateRectangle(MaterialT mat, Point origin, Point u,
                                    Point v) {
  Point normal = Normalize(CrossProduct(u, v));
  return Rectangle{
      mat, origin, u, v, normal, LengthSquared(u), LengthSquared(v)};
}

// Stuff to make a cornell box
auto MakeCornellBox() {
  Environs<8> env;

  auto red = Lambertian{{.65f, .05f, .05f}};
  auto green = Lambertian{{.12f, .45f, .15f}};
  auto white = Lambertian{{.73f, .73f, .73f}};
  auto light = Emissive{{15.f, 15.f, 15.f}};

  const float x0 = 0, x1 = 555;
  const float y0 = 0, y1 = 555;
  const float z0 = 0, z1 = 555;
  const float light_size = 130;
  const float light_pad = (x1 - light_size) / 2.f;

  env.objects[0] =
      CreateRectangle(green, {x1, y0, z0}, {0, y1 - y0, 0}, {0, 0, z1 - z0});

  env.objects[1] =
      CreateRectangle(red, {x0, y0, z0}, {0, y1 - y0, 0}, {0, 0, z1 - z0});

  env.objects[2] =
      CreateRectangle(white, {x0, y0, z1}, {x1 - x0, 0, 0}, {0, 0, -(z1 - z0)});

  env.objects[3] =
      CreateRectangle(white, {x0, y1, z0}, {x1 - x0, 0, 0}, {0, 0, z1 - z0});

  env.objects[4] =
      CreateRectangle(white, {x0, y0, z1}, {x1 - x0, 0, 0}, {0, y1 - y0, 0});

  env.objects[5] =
      CreateRectangle(light, {x0 + light_pad, y1 - 1.f, z0 - light_pad},
                      {light_size, 0, 0}, {0, 0, -light_size});
  env.objects[6] =
      Sphere{Metal{{.8f, .85f, .88f}, 0.f}, {190.f, 90.f, 190.f}, 90.f};
  env.objects[7] = Sphere{Dielectric{1.5f}, {370.f, 90.f, 370.f}, 90.f};

  Point lookfrom{278, 278, -800};
  Point lookat{278, 278, 0};
  Point vup{0, 1, 0};
  float vfov = 40.0;
  float aspect_ratio = static_cast<float>(SIZE_X) / SIZE_Y;
  auto theta = vfov * 3.14159265f / 180.f;
  auto h = std::tan(theta / 2.f);
  auto viewport_height = 2.f * h;
  auto viewport_width = aspect_ratio * viewport_height;
  auto w = Normalize(lookfrom - lookat);
  auto u = Normalize(CrossProduct(vup, w));
  auto v = CrossProduct(w, u);

  Camera cam{.lowerleftCorner = lookfrom - (viewport_width / 2.f) * u -
                                (viewport_height / 2.f) * v - w,
             .horizontal = viewport_width * u,
             .vertical = viewport_height * v,
             .origin = lookfrom};

  std::array<Colour, SIZE_X * SIZE_Y> image;
  auto image_span = std::mdspan(image.data(), SIZE_X, SIZE_Y);
}

constexpr auto CreateImage() {

  Environs<5> env;
  env.objects[0] = Sphere(Lambertian{{.1f, .2f, .5f}}, {0.f, 0.f, -1.f}, .5f);
  env.objects[1] =
      Sphere(Lambertian{{.8f, .8f, 0.f}}, {0.f, -100.5f, -1.f}, 100.f);
  env.objects[2] =
      Sphere(Metal{{0.8f, 0.6f, 0.2f}, 0.f}, {1.f, 0.f, -1.f}, .5f);
  env.objects[3] = Sphere(Dielectric{1.5f}, {-1.f, 0.f, -1.f}, 0.5f);
  env.objects[4] = Sphere(Dielectric{1.5f}, {-1.f, 0.f, -1.f}, -0.45f);

  env.objects[1] =
      CreateRectangle(Lambertian{{.5f, .5f, .5f}}, {-2.0f, -0.5f, -3.0f},
                      {4.f, 0.f, 0.f}, {0.f, 0.f, 4.f});

  constexpr Camera cam{.lowerleftCorner = {-2.f, -1.f, -1.f},
                       .horizontal = {4.f, 0.f, 0.f},
                       .vertical = {0.f, 2.f, 0.f},
                       .origin = {0.f, 0.f, 0.f}};

  constexpr auto SKY_WIDTH = SIZE_X;
  constexpr auto SKY_HEIGHT = SIZE_Y;

  std::array<Colour, SKY_WIDTH * SKY_HEIGHT> sky_texture_data{};
  for (int j = 0; j < SKY_HEIGHT; ++j) {
    for (int i = 0; i < SKY_WIDTH; ++i) {
      auto v = static_cast<float>(j) / (SKY_HEIGHT - 1);
      sky_texture_data[j * SKY_WIDTH + i] =
          (1.f - v) * Colour{1.f, 1.f, 1.f} + v * Colour{.5f, .7f, 1.f};
    }
  }

  Skybox skybox{.texture = SkySpan(sky_texture_data.data(), SIZE_Y, SIZE_X)};

  std::array<Colour, SIZE_X * SIZE_Y> image;

  auto image_span = std::mdspan(image.data(), SIZE_X, SIZE_Y);

  Render(image_span, skybox, env, cam);
  return image;
}

} // namespace

int main(int argc, char **argv) {

  /**/ constexpr auto image = CreateImage();
  auto image_span = std::mdspan(image.data(), SIZE_X, SIZE_Y);

  auto file = fmt::output_file(argv[1], fmt::file::WRONLY | fmt::file::CREATE);

  file.print("P3\n");
  file.print("{} {}\n", SIZE_X, SIZE_Y);
  file.print("255\n");

  for (int j = SIZE_Y - 1; j >= 0; --j) {
    for (std::size_t i = 0; i < SIZE_X; ++i) {
      const auto &c = image_span[i, j];
      const auto to_int = [](const float val) {
        return static_cast<int>(255.999f *
                                std::sqrt(std::max(0.0f, std::min(1.0f, val))));
      };
      const auto r = to_int(c.r);
      const auto g = to_int(c.g);
      const auto b = to_int(c.b);
      file.print("{} {} {}\n", r, g, b);
    }
  }
}
