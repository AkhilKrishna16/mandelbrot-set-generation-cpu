# Mandelbrot Set Generator (CPU)

Generates a 1920x1080 PNG image of the Mandelbrot set using parallel computation on the CPU.

![Mandelbrot Set](mandelbrot_set_2.png)

## How it works

Each pixel is mapped to a point in the complex plane and iterated through the Mandelbrot escape function up to 100 times. The iteration count determines the pixel color using a smooth coloring algorithm.

Parallelism is handled by [dispenso](https://github.com/facebookincubator/dispenso)'s `parallel_for` with `kAuto` chunking. Rows are distributed dynamically across threads using work-stealing — threads that finish cheap rows (edges of the set) pick up new work while threads computing expensive rows (center of the set, where pixels reach max iterations) continue uninterrupted.

## Performance

| Implementation | Time |
|---|---|
| pthreads (8 threads, static row partitioning) | ~0.62s |
| dispenso `parallel_for` (dynamic work-stealing) | ~0.25s |

**~2.5x speedup** from switching to dynamic load balancing. The Mandelbrot set has highly non-uniform workload — rows through the center are far more expensive than rows near the edges. Static partitioning leaves threads idle while the thread assigned to expensive center rows becomes the bottleneck.

## Build

Requires [dispenso](https://github.com/facebookincubator/dispenso) (`brew install dispenso` on macOS).

```sh
g++ -std=c++17 -O2 -o mandelbrot-generate mandelbrot-generator.cpp \
  -I/opt/homebrew/opt/dispenso/include \
  -I/opt/homebrew/opt/dispenso/include/dispenso/third-party \
  -L/opt/homebrew/opt/dispenso/lib \
  -ldispenso -lm -lpthread
```

## Run

```sh
./mandelbrot-generate
```

Outputs `mandelbrot_set_2.png` in the current directory.
