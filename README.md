# C++23 Compile Time Raytracer

<img src="images/compile_time_image.png" alt="A ray-traced scene with spheres" width="600"/>


Compile time version of RaytracingInOneWeekend code, proving that just because you can do something doesn't mean you should.

Compile time for me takes around a minute on a 12900k for a 100 x 50 image.

On the other hand if you want to make the image larger and add more rays per pixel and depth, remove the constexpr and it performs pretty well.

Outputs the file to a ppm format

## Dependencies
I have included the header only version of mdspan as some compilers do not include it.

You will also need fmt installed.

## Build command (Make sure you have a modern compiler)
```bash
g++ -std=c++23 -O3 -fconstexpr-ops-limit=4294967295 -march=native main.cpp -lfmt
```

## Run
```bash
./a.out MyImage.ppm
```
