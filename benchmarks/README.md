# Vulkyrie benchmarks

Timing harness for the engine, built on [Catch2's microbenchmarking
support](https://github.com/catchorg/Catch2/blob/devel/docs/benchmarks.md). Separate binary from
`tests`: a test asks *is this correct*, a benchmark asks *how fast is this*, and only one of those
belongs on the path of every `ctest` run. Nothing here is registered with CTest.

## Running

Benchmarks are opt-in (`VULKYRIE_BUILD_BENCHMARKS`, default `OFF`); the `benchmarks-*` and `all-*`
presets turn them on.

```bash
cmake --preset clang-benchmarks-release
cmake --build --preset clang-benchmarks-release

build/clang-benchmarks-release/benchmarks/benchmarks              # everything (~20 s)
build/clang-benchmarks-release/benchmarks/benchmarks "[jobs]"     # one subsystem
build/clang-benchmarks-release/benchmarks/benchmarks "[scaling]"  # only the worker-count sweeps
build/clang-benchmarks-release/benchmarks/benchmarks --list-tests
```

**Always measure a Release build.** A Debug number describes the unoptimized build, not the design,
and the sweeps take minutes instead of seconds. `all-debug` builds this target only so it cannot rot.

Useful Catch2 flags: `--benchmark-samples N` (default 100 — lower it while iterating),
`--benchmark-warmup-time MS`, and `--reporter xml` for machine-readable output. Engine log lines are
interleaved with the results on purpose — a benchmark quietly hitting a slow path (an exhausted
pool, a failed core pin) is worse than no benchmark — so pipe through `grep -v INFO` if they are in
the way.

## Layout

```
benchmarks/src/
  support/                    scaffolding shared by every benchmark, whatever it measures
    benchmark_support.h         BusyWork, JobSystemScope, ScalingWorkerCounts, WorkerLabel
    benchmark_listener.cpp      brings engine subsystems up once per run
  core/jobs/                  mirrors the engine's module layout, same as tests/src/
```

Sources are globbed (`CONFIGURE_DEPENDS`), so a new file is picked up without touching CMake.

## Adding benchmarks for another subsystem

1. Create `benchmarks/src/<module>/<thing>_benchmarks.cpp`, mirroring the engine's layout — e.g.
   `benchmarks/src/physics/collision/dynamic_aabb_tree_benchmarks.cpp`.
2. Include the scaffolding as `#include "support/benchmark_support.h"` (the include path is rooted
   at `src/`, so this works at any depth), plus `<catch2/benchmark/catch_benchmark.hpp>` and
   `<catch2/catch_test_macros.hpp>`.
3. Tag the `TEST_CASE` with the subsystem (`"[physics]"`) so it can be run on its own, and add
   `[scaling]` to worker-count sweeps.
4. If the subsystem needs its own one-time setup, add it to `benchmark_listener.cpp` rather than to
   individual benchmarks — everything measured should start from the engine's normal running state.

```cpp
#include "support/benchmark_support.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Vulkyrie;

TEST_CASE("Broadphase insertion", "[physics]") {
    BENCHMARK("Insert 1000 proxies") {
        // Return something derived from the work: a body whose result is discarded can be
        // optimized away entirely, and the benchmark will happily report the time to do nothing.
        return DoTheWork();
    };

    // Setup that must not be timed goes outside meter.measure().
    BENCHMARK_ADVANCED("Query a populated tree")(Catch::Benchmark::Chronometer meter) {
        auto tree = BuildTree();
        meter.measure([&tree] { return tree.Query(...); });
    };
}
```

## Writing benchmarks that mean something

- **Consume the result.** Return it from the benchmark body, or the optimizer deletes the work.
  `Bench::BusyWork` exists for this: its rounds are serially dependent, so nothing vectorizes or
  folds away, and it returns a value you can feed back.
- **Compare against a baseline in the same file.** A number in isolation is noise; `687 us serial
  vs 63 us at 15 workers` is a result. The `[scaling]` cases always include a `synchronous` row.
- **Keep setup out of the measured region** with `BENCHMARK_ADVANCED` + `Chronometer::measure`.
- **Stay inside the pools.** The default job pool is 8192 slots; a benchmark that exhausts it
  measures the exhaustion-recovery path instead of the thing under test. Use `Bench::JobSystemScope`
  with an explicit `JobSystemConfig` if a benchmark genuinely needs bigger pools.
- **Reconfigure through `Bench::JobSystemScope`**, never by calling `JobSystem::Shutdown` directly:
  it restores the run listener's configuration on the way out, so a throwing benchmark cannot leave
  every later one measuring the wrong pool.
