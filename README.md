# 🐦 Blue-Footed Booby - Monte Carlo Path Tracer (C++)

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

- ⚡ **10.2× faster** with C++17 parallel execution - a 128 spp frame from **19.1 s → 1.9 s** (AMD Ryzen 7 9700X, 8-core / 16-thread)
- 🌐 **Monte Carlo global illumination** - soft shadows and color bleeding *emerge* from the rendering equation
- 🔬 **Physically-based materials** - dielectric glass (Fresnel + refraction + total internal reflection) and metal (mirror + roughness)
- 🎚️ **Cosine-weighted importance sampling**, Reinhard tone-mapping + gamma
- 🖥️ Real-time **DirectX 11** viewport with live debug views (normals / depth / albedo)

---

## Performance

> **[Phase 2]** Parallelised with C++17 parallel algorithms (`std::execution::par`) for a **10.2×
> speedup** - a 128 spp frame dropped from **19.1 s to 1.9 s** on an AMD Ryzen 7 9700X (8-core /
> 16-thread). Next: a hand-built **tile-based thread pool** to replace the standard algorithm and
> compare the two.

### Render time vs. samples-per-pixel
Measured with a `steady_clock` harness (600×600, max depth 8, Release build, AMD Ryzen 7 9700X).
Both columns are the same scene on the same machine - and because the per-pixel RNG is seeded
deterministically from `(x, y, sample)`, the parallel run produces an **identical image**, which is
what makes the comparison meaningful rather than just faster.

| spp | 1 thread | 16 threads | Speedup |
|---:|---:|---:|---:|
| 1 | 0.52 s | 0.045 s | 11.6× |
| 2 | 0.65 s | 0.058 s | 11.2× |
| 4 | 0.93 s | 0.085 s | 10.9× |
| 8 | 1.48 s | 0.143 s | 10.3× |
| 16 | 2.57 s | 0.250 s | 10.3× |
| 32 | 4.90 s | 0.486 s | 10.1× |
| 64 | 9.44 s | 0.945 s | 10.0× |
| 128 | 19.09 s | 1.865 s | **10.2×** |

### From single-threaded to parallel - the reasoning
The 1/√N curve above *is* the problem, in one line: a clean image needs **many** samples, and each
one costs the same. So I started where any optimization should - by **measuring**, not guessing. The
single-threaded baseline was **19 s for a 128 spp frame**, and a genuinely clean hero image wants
closer to 1024 spp - minutes per render. That breaks the tight edit → render → look loop this kind
of work lives on.

Two observations made the fix clear:

- **Path tracing is parallel.** Every pixel is an independent estimate; no pixel needs
  another pixel's result. There is nothing to coordinate except handing out the work.
- **The data structure design was already lock-free.** Each pixel seeds its own RNG from `(x, y, sample)`, so there
  is no shared mutable state on the hot path. There is nothing to lock. A welcome consequence: the parallel
  image comes out **bit-identical** to the serial one, which is exactly how I verified the
  parallelization was correct (a diff of the two buffers must be zero).

So the first step was the pragmatic one - `std::execution::par` across the image rows - for the
**10.2×** in the [Performance](#performance) table. The *next* step, a hand-built tile-based thread
pool, is less about more speed and more about understanding the machinery `par` hides: a work queue,
`std::mutex`, `std::condition_variable`, and balancing tiles of uneven cost (the glass sphere is far
heavier than an empty wall). Reaching for the standard algorithm first and the manual implementation
second is deliberate: ship the result, then go learn the layer underneath it.



### Next: a hand-built tile-based thread pool
`std::execution::par` was the pragmatic win; the thread pool is the interesting engineering, and
having both makes the comparison the point. Planned design: worker threads pull 32×32 tiles off a
shared work queue (`std::mutex` + `std::condition_variable`). A **work queue rather than a static
split** because some tiles (the glass sphere) are far slower than others - dynamic distribution
balances the load. The per-pixel deterministic RNG means **no locks on the pixel-write path**.

_A tile-progress GIF (tiles completing in parallel across worker threads) will go here once the pool
is built._

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

## The Life of A Pixel

This is the life of a single pixel, start to finish.

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

**The core idea:** the color of a pixel is an **integral** over every path light could take to
reach it, which is impossible to solve directly. So I estimate it with **Monte Carlo**, tracing many random
light paths and calculating the average. Global illumination, soft shadows, and color bleeding all *emerge* from
this.

---

## Technical Highlights

### Monte Carlo integration of the rendering equation
Each pixel is an integral over all incoming light; I estimate it by averaging random paths. Noise
falls as **1/√N**, so halving it costs **4×** the samples - the central performance tension of the
whole renderer. The integrator itself is small - it *is* the rendering equation, written as a single
unbranching recursive path:

```cpp
// One random, unbranching light path (backward, from the camera).
math::vec3 tracePath(const Ray ray, int depth, RNG& rng)
{
    if (depth <= 0) return math::vec3(0.0f);   
    Hit hit = FindClosestCollision(ray);
    if (hit.distance < 0.0f) return math::vec3(0.0f);   

    math::vec3 emitted = hit.obj->material->Emitted();

    Ray scattered;  math::vec3 attenuation;
    if (hit.obj->material->Scatter(ray, hit, rng, attenuation, scattered))
        return emitted + attenuation * tracePath(scattered, depth - 1, rng);
    return emitted;                                     
}
```

<p align="center">
  <img src="./resources/Cornell%20Box%20-%20spp%201.png" alt="1 sample per pixel" width="195"/>
  <img src="./resources/Cornell%20Box%20-%20spp%2016.png" alt="16 samples per pixel" width="195"/>
  <img src="./resources/Cornell%20Box%20-%20spp%20128.png" alt="128 samples per pixel" width="195"/>
  <img src="./resources/Cornell%20Box%20-%20spp%201024.png" alt="1024 samples per pixel" width="195"/><br>
  <sub><i>The same scene at <b>1 / 16 / 128 / 1024</b> samples per pixel - Monte Carlo noise converging as 1/√N.<br>
  Every other parameter is held fixed; only the sample count changes. Each step is 8× the samples for
  ~2.8× less noise - visibly diminishing returns, and the reason the 10× speedup mattered.</i></sub>
</p>

### Materials as a polymorphic interface
`Material` is an abstract base with two virtual methods: `Scatter()` (how a ray bounces off the
surface) and `Emitted()` (light the surface emits). `Lambertian`, `Metal`, `Dielectric`, and
`DiffuseLight` implement them. Geometry and shading are decoupled: each `Object` holds a `Material*`,
and `tracePath` calls `Scatter()` / `Emitted()` through the base pointer without branching on the
material type.

The tradeoff is a virtual call per ray-surface hit. A data-oriented approach - grouping hits by
material and shading each group in one loop - removes that indirection, which is what production
renderers do.

```cpp
class Material {
public:
    virtual math::vec3 Emitted() const { return math::vec3(0.0f); }
    virtual bool Scatter(const Ray& in, const Hit& hit, RNG& rng,
                         math::vec3& attenuation, Ray& scattered) const = 0;
};

// Matte surface: bounce in a cosine-weighted random direction.
class Lambertian : public Material {
    math::vec3 albedo;
public:
    bool Scatter(const Ray& in, const Hit& hit, RNG& rng,
                 math::vec3& attenuation, Ray& scattered) const override {
        math::vec3 dir = hit.normal + rng.randomUnitVector();  // → cosine lobe
        scattered   = { hit.point + hit.normal * 1e-3f, math::normalize(dir) };
        attenuation = albedo;                                  // the PDF cancels the cosine term
        return true;
    }
};
```

### Glass: Fresnel, refraction, and total internal reflection
The dielectric picks reflection or refraction per sample, weighted by the **Fresnel** reflectance
(Schlick approximation), handles **total internal reflection**, and correctly switches between
air→glass and glass→air. Averaged over samples, it reproduces the correct reflect/refract blend.

### From HDR light to a displayable image (tone-map + gamma)
The renderer works in unbounded **linear light**, and compresses to a displayable image only at the
very end - **Reinhard tone-mapping** (so bright highlights don't clip to flat white) followed by
**gamma correction** (so midtones aren't crushed). Skipping it leaves the raw render dark and muddy:

<p align="center">
  <img src="./resources/Cornell%20Box%20Stage%203%20-%20Initial%20Path%20Tracing.jpg" alt="Raw linear render, dark" width="360"/>
  <img src="./resources/Cornell%20Box%20Stage%205%20-%20tone%20mapping%20and%20gamma%20%28spp%20128%29.jpg" alt="After tone-mapping and gamma" width="360"/><br>
  <sub><i>Before (raw linear output) → after (Reinhard tone-map + gamma).</i></sub>
</p>

---

## Roadmap
- ✅ **Parallel rendering** - `std::execution::par`, **10.2× speedup** ([Performance](#performance))
- **Hand-built tile-based thread pool** - replace the standard algorithm with a work queue
  (`std::mutex` + `std::condition_variable`) and chart the scaling curve across thread counts
- **Next-event estimation** - sample the light directly for much less noise
- **Golden-image regression tests + CI** - verify renders don't drift

---

## References
- *Ray Tracing in One Weekend* - Peter Shirley
- The Cornell Box - Cornell University Program of Computer Graphics
