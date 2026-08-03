# 🐦 Blue-Footed Booby - Monte Carlo Path Tracer (C++)

[![CI](https://github.com/labcows/BlueFootedBooby/actions/workflows/ci.yml/badge.svg)](https://github.com/labcows/BlueFootedBooby/actions/workflows/ci.yml)

> A physically-based renderer built **from scratch** in C++ - no rendering libraries - that
> simulates real light transport (global illumination, soft shadows, glass, metal).

<p align="center">
  <img src="./resources/Cornell%20Box%20-%20spp%201024%20depth%2016.png" alt="Cornell Box path-traced render with glass and metal spheres" width="640"/><br>
  <sub><i>Cornell Box - path-traced with global illumination, a glass sphere, and a metal sphere.<br>
  1024 samples/pixel, max path depth 16, 1200×1200 - a sample budget the 10× parallel speedup made affordable.</i></sub>
</p>

---

## Project Summary

I do not have to tell my eyes how to render the world. They just do it for me automatically - the way a light source delicately interacts with every
material around it, moment to moment. I kept coming back to one question: **how would I draw *that*
from first principles?** Not "how do I use a game engine?" but *why* light looks the way it does. I
built this to answer that question for myself.

**Blue-Footed Booby is a Monte Carlo path tracer written from scratch in modern C++**, rendering the
classic Cornell Box with physically-based light transport. "From scratch" means I programmed every
ray/light-transport line. The dependencies are **GLM** (vector math), **Dear ImGui**
(debug UI), and **DirectX 11** - which merely *displays* the finished CPU-rendered frame, a GPU
renderer being on the roadmap in the future.

- ⚡ **~10× faster** with my own tile-based thread pool - a 128 spp frame at 1200×1200 from **77 s → 7.8 s** (AMD Ryzen 7 9700X, 8-core / 16-thread), verified pixel-identical to the serial render
- 🌐 **Monte Carlo global illumination** - soft shadows and color bleeding *emerge* from the rendering equation
- 🔬 **Physically-based materials** - dielectric glass (Fresnel + refraction + total internal reflection) and metal (mirror + roughness)
- 🎚️ **Cosine-weighted importance sampling**, Reinhard tone-mapping + gamma
- 🖥️ Real-time **DirectX 11** viewport with live debug views (normals / depth / albedo)

---

## Performance

> **[Phase 2]** A 128 spp frame at 1200×1200 renders in **7.8 s instead of 77 s** - a **~10×
> speedup** on an AMD Ryzen 7 9700X (8-core / 16-thread). Two parallel back-ends are implemented and
> benchmarked against each other: C++17 `std::execution::par`, and a **hand-built tile-based thread
> pool**.

### Deciding to multithread after one frame took 77 seconds
A clean image needs many samples per pixel, and every sample costs the same. On one thread a 128 spp
frame took **77 seconds**, and a genuinely clean image wants 1024 spp - roughly ten minutes per
render. At that speed, every edit-compile-render-inspect cycle costs more time than the change itself.

Two properties of the renderer made parallelising it straightforward:

- **Every pixel is independent.** A pixel's colour never depends on another pixel's result, so there
  is nothing to coordinate beyond handing out the work.
- **There is no shared state to lock.** Each pixel seeds its own random number generator from
  `(x, y, sample)`, so no two threads ever touch the same data.

### Comparing three ways to render the same frame

Measured with a `steady_clock` harness (1200×1200, max depth 8, Release build, Ryzen 7 9700X,
16 worker threads, 32×32 tiles).

Each cell is wall-clock render time; the figure in parentheses is the speedup over the serial run.

| spp | Serial (1 thread) | `std::execution::par` | Tiled thread pool | Pixel diff |
|---:|---:|---:|---:|---:|
| 1 | 2.034 s | 0.179 s (11.35×) | 0.175 s (11.59×) | 0 |
| 4 | 3.807 s | 0.351 s (10.85×) | 0.348 s (10.93×) | 0 |
| 16 | 10.629 s | 1.027 s (10.35×) | 1.017 s (10.45×) | 0 |
| 64 | 39.083 s | 3.863 s (10.12×) | 3.863 s (10.12×) | 0 |
| 128 | 77.249 s | 7.733 s (9.99×) | 7.784 s (**9.92×**) | 0 |

The last column checks correctness. Because the random number generator is seeded from the pixel
coordinate, dividing the work differently must not change a single pixel, so the benchmark also
compares the three framebuffers against each other. All 1.44 M pixels match exactly in every run. A
data race would show up here as a scattering of differing pixels, which a benchmark that only
measures time would miss.

The hand-built pool lands within ~1% of `std::execution::par` (backed by Intel TBB on MSVC) - a gap
small enough to be measurement noise. Most of the gain comes from using all cores at all; the
scheduling strategy on top, rows versus 32×32 tiles, matters less on this scene, where even a single
1200-pixel row already mixes cheap wall and expensive glass.

### Speedup from 1 to 16 threads

This table is the reason for writing the pool by hand: `std::execution::par` gives no control over
thread count, so it cannot produce these measurements at all. (64 spp, 1200×1200.)

| threads | time | speedup | efficiency |
|---:|---:|---:|---:|
| 1 | 37.851 s | 1.00× | 100.0% |
| 2 | 24.064 s | 1.57× | 78.7% |
| 4 | 12.652 s | 2.99× | 74.8% |
| 8 | 6.446 s | 5.87× | 73.4% |
| 12 | 4.594 s | 8.24× | 68.7% |
| 16 | 3.819 s | 9.91× | 61.9% |

Efficiency is measured against the single-threaded run, which is the fastest case per thread and so
sets a demanding baseline. The steepest drop is the very first step; after that the curve flattens
out, and from 2 to 16 workers - 8× the threads - throughput still rises **6.3×**. The CPU has 8
physical cores exposing 16 hardware threads, so the last workers share cores rather than adding new
ones, which is where the remaining efficiency goes.

### Spending the speedup on image quality
The point was never a shorter wait. Monte Carlo noise falls as `1/√N`, so the time that previously
bought 128 spp now buys **1024 spp - a 2.8× cleaner render**, which is what the image at the top of
this page is. Interactive debug views stay at a low sample count for instant feedback, and the
sample budget goes where it shows.

_A tile-progress GIF (tiles completing in parallel across worker threads) is still to come._

---

## Gallery

<p align="center">
  <img src="./resources/Cornell%20Box%20Stage%206%20-%20Apply%20clean%20metal%20material%20to%20the%20back%20sphere%20%28spp%20128%29.jpg" alt="Metal sphere mirroring the room" width="390"/>
  <img src="./resources/Cornell%20Box%20Stage%207%20-%20Apply%20glass%20metal%20material%20to%20the%20front%20sphere%20%28spp%20128%29.jpg" alt="Glass and metal spheres in the Cornell Box" width="390"/><br>
  <sub><i>Left: the metal sphere mirrors the room. Right: the glass sphere added - note the red/green <b>color bleeding</b> on the floor from indirect light.</i></sub>
</p>

<p align="center">
  <img src="./resources/Debug-Normal.jpg" alt="Surface-normals debug view" width="390"/>
  <img src="./resources/Draw%20one%20rectangle.jpg" alt="Early rectangle-intersection test" width="390"/><br>
  <sub><i>Building with verification: the <b>normals</b> debug view (left) confirms geometry orientation before any lighting; an early rectangle-intersection test (right).</i></sub>
</p>

---

## How a single pixel gets its colour

Start to finish, for one pixel:

1. **Cast a ray** from the camera through the pixel (pinhole camera).
2. **Sample many times** - fire N jittered rays per pixel (anti-aliasing + Monte Carlo sampling).
3. **Find the nearest surface** - where the ray hits.
4. **Scatter or Emit** - the surface's material decides how the ray bounces (diffuse / mirror /
   glass) and whether it glows.
5. **Follow the path** - recurse along the bounce, gathering light:
   `emitted + albedo × (light from the rest of the path)`.
6. **Average** the N samples → an estimate of the true light reaching that pixel.
7. **Tone-map + Gamma** the linear HDR result down to a displayable color.
8. **Display** via DirectX 11.

The colour of a pixel is an integral over every path light could take to reach it, which cannot be
solved directly. It is estimated instead with **Monte Carlo** sampling: trace many random light paths
and average them. Global illumination, soft shadows, and colour bleeding are not special-cased
anywhere - they fall out of this.

---

## How the renderer is tested

A renderer fails quietly. A wrong normal or a lost sample does not crash anything. It just makes the
image slightly wrong, and slightly wrong is hard to notice by eye. The tests are built around that.

**1. Unit tests** cover the parts with exact, hand-checkable answers: ray-sphere and ray-rectangle
intersection (front face, miss, geometry behind the ray, and a ray starting *inside* a sphere, which
is the case glass depends on), tile decomposition, and the thread pool - which is checked for running
every submitted job exactly once, blocking until all of them finish, and surviving reuse across
batches.

**2. A golden-image regression test** renders a small Cornell Box frame and compares all 27,648 bytes
against a reference image committed to the repo. A second test intentionally changes a known number
of bytes and asserts that exactly that many are reported - a regression test that cannot fail is
worth nothing, so its ability to fail is itself tested.

**3. CI** builds the renderer and the tests from scratch on a clean Windows runner and runs the suite on
every push.

---

## Roadmap
- ✅ **Parallel rendering** - `std::execution::par` and a hand-built tile-based thread pool
  (`std::mutex` + `std::condition_variable` + work queue), **~10× speedup**, benchmarked against each
  other and verified pixel-identical ([Performance](#performance))
- ✅ **Golden-image regression tests + CI** - every push builds and tests on a clean runner
- **Next-event estimation** - sample the light directly for much less noise

---

## References
- *Ray Tracing in One Weekend* - Peter Shirley
- The Cornell Box - Cornell University Program of Computer Graphics
