# Vulkyrie Physics Engine — Performance & Parallelism Architecture

## Context

The Vulkyrie physics engine (`engine/{include,src}/physics/`) is a faithful, well-structured
single-threaded port of ReactPhysics3D: SoA component stores, dynamic-AABB-tree broadphase,
GJK/SAT + specialized narrowphase, island grouping, and sequential-impulse (Gauss-Seidel)
constraint/contact solvers with warm starting and split-impulse/Baumgarte position correction.

The code is clean and correct but leaves **all** modern-hardware performance on the table. The
goal is to evolve it toward an enterprise-grade engine (think Jolt / Box2D v3 / PhysX class)
where performance is paramount: multi-core CPU scaling, SIMD, cache-friendly data, optional GPU
offload, and better numerical robustness — without throwing away the solid ECS/SoA foundation.

This document is the **architecture blueprint + phased roadmap**. It is design-first; each phase
lists the concrete files/patterns to touch so it can be executed incrementally with the test
suite (`tests/src/physics/`) guarding correctness at every step.

---

## Current-state analysis (what the profiling model tells us)

**Execution model — fully serial.** `PhysicsWorld::Update()` (`physics_world.cpp:54`) runs a
strict serial pipeline: `ComputeCollisions → createIslands → CreateContacts → integrate →
solve → integrate positions → position-correct → UpdateStates → UpdateColliders → sleep`.
There is **no threading, no SIMD, no GPU, and no job system anywhere in the engine** (confirmed
by grep across `engine/`). Everything runs on one core.

**Islands are computed but never exploited for parallelism.** `createIslands()`
(`physics_world.cpp:419`) already partitions awake bodies into independent islands (`Islands`,
`types/islands.h`), but the contact solver (`contact_solver_system.cpp:69`) and joint solvers
(`constraint_solver_system.cpp:28`) iterate **flat global arrays**. Islands are used only for
sleeping and manifold grouping. Independent islands are the single biggest, lowest-risk source
of parallelism and are being thrown away.

**Solver is scalar Gauss-Seidel over `glm::vec3`.** `ContactSolverSystem::Solve` and the four
joint solver systems recompute per-iteration cross products and `mat3 * vec3` on scalar floats.
No batching, no SIMD. `VelocitySolverIterations = 6`, `PositionSolverIterations = 3`
(`physics_world_settings.h`).

**Data layout is SoA but lookup-heavy.** Component stores (`component_store.h`,
`rigid_body_component_store.h`) keep parallel arrays (good) but every entity-keyed accessor does
an `std::unordered_map<Entity,size_t>` hash lookup. Hot loops mostly use `*AtIndex` variants, but
several per-step passes still hash per body — e.g. `updateBodiesInverseWorldInertiaTensors()`
(`physics_world.cpp:629`) calls `GetTransform(entity)` (a map lookup) per active body;
`createIslands`/`updateSleepingBodies` do likewise. `glm::vec3`/`mat3` are stored AoS-per-field,
so the layout is not SIMD-lane-friendly.

**Broadphase is a single non-thread-safe tree.** `DynamicAABBTree` uses a shared `_queryNodesToVisit`
scratch buffer (explicitly documented non-thread-safe, `dynamic_aabb_tree.h:270`).
`ComputeOverlappingPairs` and pair updating run serially.

**Narrowphase has a batch structure it never parallelizes** (`NarrowPhaseInput` /
`NarrowPhaseDataBatch`), and it churns the heap: concave-collision triangle shapes are
`new`/`delete`d per pair (`narrow_phase_data_batch.h:55` Dispose), and per-frame contact bookkeeping
rebuilds `std::unordered_map`s (`collision_system.h:129`).

**Stepping is non-fixed and non-substepped.** The sandbox calls `world.Update(deltaTime)` with the
raw frame delta (`sandbox_layer_sphere.h:164`) — no fixed-timestep accumulator, no substepping,
no CCD. That hurts stability/determinism independent of performance.

**Foundational gaps for "enterprise" grade:** no job system (reusable by the renderer too), no
SIMD math path, no memory arenas/pools (per-frame `std::vector` churn), no built-in profiling
counters wired into the physics step (a `Profiler` exists at `debug/profiler.h` but is unused
here), no CCD, no broadphase/solver determinism guarantees.

---

## Locked decisions

1. **CPU-first**; GPU compute is a designed future phase (Phase 7, design-only).
2. **Custom lightweight job system** built in-engine (reused by the renderer later).
3. **Cross-platform determinism is a hard requirement** (targets lockstep netcode / replays).
4. **Modernize the solver** to TGS-Soft + substepping with a graph-colored parallel island solver.

### Why determinism and parallelism are compatible here

Bit-reproducibility usually fights multithreading because unordered float reductions aren't
associative. This design avoids that by construction:

- **Islands are physically independent** — assigning them to threads in any order cannot change a
  result, only *reporting* order, which we normalize by sorting outputs by stable keys.
- **Graph coloring** partitions an island's constraints into colors where **no two constraints in
  the same color share a body**. Applying a color's impulses in parallel touches disjoint memory,
  so the result is identical regardless of thread interleaving. Colors run sequentially.
- **No cross-body atomic accumulation in the math path.** Reductions (e.g. island energy for
  sleeping) use fixed-order passes over sorted arrays, not `atomic` add.
- **Determinism discipline** (see cross-cutting section) pins float evaluation: `-ffp-contract=off`,
  no `-ffast-math`, SSE2/AVX only (no x87), engine-owned transcendentals, stable sorts, and
  stable feature IDs for warm-start matching. This is the price of the requirement and it is
  enforced project-wide, not per-file.

---

## Target architecture

The per-step pipeline becomes a **task graph** driven by a fixed-timestep accumulator with
substepping. Stages that are embarrassingly parallel (integration, broadphase leaf updates,
narrowphase pairs, per-island/per-color solve) run on the job system; stages that must be serial
(island assembly, final pair ordering) stay serial but cheap.

```
Update(frameDt):
  accumulator += frameDt
  while accumulator >= fixedDt:          # fixed-step, deterministic
      Step(fixedDt)
      accumulator -= fixedDt
  interpolationAlpha = accumulator / fixedDt   # for render interpolation

Step(dt):
  BroadPhase   (parallel tree refit + parallel pair query -> deterministic sorted pairs)
  NarrowPhase  (parallel_for over pair batches -> per-thread contact arenas -> deterministic merge)
  BuildIslands (serial DFS; then color each large island's constraint graph)
  Solve        (for each substep: integrate -> warmstart -> [parallel over islands,
                                    parallel over colors within island] TGS-Soft iterations
                                    -> relax) 
  Finalize     (parallel integrate positions, refit collider AABBs, sleeping)
```

---

## Phased roadmap

### Phase 0 — Measurement & determinism harness (do this first)

You cannot optimize what you cannot measure, and you cannot keep determinism you cannot detect.

- Wire the existing `Profiler` (`debug/profiler.h`) into `PhysicsWorld::Update` with a scoped
  timer per stage (broadphase, narrowphase, islands, solve, integrate, sleep). Add per-stage
  counters (pairs, manifolds, contacts, islands, largest island).
- Add a **benchmark scene** (a new `examples/` or `tests/` target): stacks, piles, and a large
  box pyramid parameterized by body count (1k → 100k), plus a joint-heavy scene (ragdolls).
- Add a **determinism test**: run N identical steps twice (and single- vs multi-threaded once the
  job system lands), hash the full body state (`position, orientation, lin/ang velocity`) each
  step via a stable FNV/xxHash over sorted-by-entity arrays, and assert equality. This test is
  the gate for every subsequent phase.

Files: new `tests/src/physics/determinism_tests.cpp`, `tests/src/physics/benchmarks/`, minor edits
to `physics_world.cpp`.

### Phase 1 — Deterministic fixed-step foundation + hot-path data cleanup

Single-threaded wins that are also prerequisites for TGS-Soft and determinism.

- **Fixed timestep + substep accumulator.** Introduce a stepping layer (either in `PhysicsWorld`
  or a thin `PhysicsStepper`) that decouples render `frameDt` from a fixed `fixedDt`, runs an
  integer number of `Step()`s, and exposes an interpolation alpha. Update the sandbox
  (`sandbox_layer_sphere.h:164`) to use it. This alone removes the biggest determinism/stability
  hole (raw variable-dt integration).
- **Kill per-step map lookups on hot paths.** In `updateBodiesInverseWorldInertiaTensors`,
  `updateSleepingBodies`, `createIslands`, and `DynamicsSystem::UpdateStates`, replace
  `GetTransform(entity)` / `GetEntityIndex(entity)` hashing with index-parallel access. Concretely:
  store the transform SoA co-indexed with the rigid-body store (or cache the component index on
  the body), so the store loop is pure `[i]` indexing. Add `*AtIndex` transform accessors mirroring
  the rigid-body store.
- **Stable ordering everywhere that feeds the solver.** Sort broadphase pairs and contact
  manifolds by `(minEntityId, maxEntityId, featureId)` before they are consumed, so downstream
  order is independent of tree layout / hash iteration.
- **Establish the determinism build discipline** (see cross-cutting) in `Dependencies.cmake` /
  compiler flags.

### Phase 2 — Custom job system (`core/jobs/`)

Engine-wide infrastructure, not physics-specific.

- `ThreadPool` with one worker per hardware thread (minus main), **work-stealing deques**, and
  affinity pinning (pinning aids determinism and cache locality).
- `parallel_for(count, grainSize, fn)` with **deterministic static partitioning** (fixed range →
  worker mapping given a fixed thread count) so results don't depend on steal timing. Provide a
  `parallel_for` variant that writes into **per-worker output buffers** merged in worker-index
  order.
- A minimal **task graph / `JobHandle` + dependencies** so the `Step()` stages can express
  producer/consumer edges and the scheduler overlaps independent work.
- Design note for determinism: the *scheduling* is nondeterministic but every *result-affecting*
  reduction uses fixed partition boundaries + fixed merge order, so outputs are reproducible.

Files: new `engine/include/core/jobs/` + `engine/src/core/jobs/`; `tests/src/core/jobs/`.

### Phase 3 — Parallel collision detection

- **Broadphase.** Give the AABB tree per-thread query scratch (remove the shared
  `_queryNodesToVisit`, `dynamic_aabb_tree.h:272`). Refit/update moved leaves in `parallel_for`.
  Generate overlapping pairs in parallel (each moved leaf queries the tree into a per-thread pair
  list), then **concatenate in worker order and stable-sort** → deterministic pair set. Keep the
  incremental "fat AABB" scheme. (Future option: SAP or a two-tree static/dynamic split; note it,
  don't build it yet.)
- **Narrowphase.** Run `computeNarrowPhase` as a `parallel_for` over `NarrowPhaseDataBatch`
  entries; each worker emits `ContactPointData`/`ContactManifoldData` into a **per-worker arena**,
  merged deterministically. **Remove per-pair heap churn**: pool the concave triangle shapes
  (`narrow_phase_data_batch.h:55`) in a per-worker freelist instead of `new`/`delete`; reuse the
  `unordered_map` bookkeeping via a frame arena or a flat, index-based map.
- **Manifold persistence / warm-start matching** must key on **stable feature IDs** (not pointers)
  so warm starting is deterministic across threads and frames.

### Phase 4 — Modern parallel solver (TGS-Soft + substepping + graph coloring)

The core change. Replaces the Baumgarte/split-impulse machinery
(`contact_solver_system.cpp`, the four `*_solver_system.cpp`) with a soft, substepped solver.

- **TGS-Soft formulation.** Solve constraints with soft parameters (per-constraint
  stiffness/damping → biasRate, massScale, impulseScale) and **relax** passes, integrating
  positions *inside* the substep loop so the Jacobians track the moving geometry (temporal
  Gauss-Seidel). This removes the split-impulse hack and the separate position-solver loop while
  giving stiffer stacks and better joints. Substep count (e.g. 4) replaces most velocity
  iterations.
- **Island-level parallelism.** `Solve` dispatches islands across the job system — each island is
  independent, so this is trivially parallel and deterministic.
- **Graph coloring inside large islands.** For an island above a threshold, greedily color its
  contact+joint graph so each color is a set of body-disjoint constraints; solve colors
  sequentially, constraints **within a color in parallel**. Colors are built with a deterministic
  greedy order (sorted by stable constraint key). Small islands skip coloring and solve serially
  (cheaper).
- **Unified constraint representation.** Fold contacts and all four joint types into a common
  SoA constraint block layout so the solver loop, coloring, and SIMD batching are shared code
  rather than five bespoke loops (`constraint_solver_system.cpp:28` currently calls each joint
  type serially in turn).
- **Parallel integration.** `IntegrateVelocities` / `IntegratePositions` / `UpdateStates`
  (`dynamics_system.cpp`) become `parallel_for` over the active-body SoA (already index-friendly).

### Phase 5 — SIMD wide solver + memory arenas

- **Wide constraint solving.** Lay out colored constraint batches as **SoA-of-N** (4-wide SSE /
  8-wide AVX2) and solve N independent constraints per SIMD register — the classic wide
  sequential-impulse/TGS kernel. Because a batch is body-disjoint (from coloring), lanes never
  alias. Gate AVX2 behind runtime CPU detection with an SSE2 fallback (SSE2 is the deterministic
  baseline; keep the *same* math in both, only the width changes, and fix the reduction order so
  4-wide and 8-wide agree — or pick one width as the canonical deterministic path).
- **Memory.** Introduce per-frame linear **arena allocators** and **object pools** for solver
  constraint blocks, contact data, island scratch, and narrowphase temporaries — replacing the
  many per-frame `std::vector` clears/reallocs. Align hot SoA arrays to cache lines / SIMD width.
- Consider migrating hot `glm::vec3` SoA arrays to explicit `x[]/y[]/z[]` planar SoA where the
  SIMD kernels want it (keep `glm` at the API boundary).

### Phase 6 — Continuous collision & robustness

- **Speculative contacts** (cheap, solver-integrated CCD): inflate broadphase by
  velocity*dt and let the solver see contacts before penetration — pairs well with substepping.
- Optionally **conservative advancement** for bullet-like fast bodies flagged for full CCD.
- Restitution-with-substeps handling, contact-softness materials, and improved sleeping
  (island-coherent, already partially present in `updateSleepingBodies`).

### Phase 7 — GPU compute path (design only, future)

Documented target, not built now (waiting on the Vulkan backend to mature):

- GPU broadphase (parallel LBVH build or uniform grid) and a GPU XPBD/TGS solver in compute
  shaders, with double-buffered body state and async readback.
- **Determinism caveat:** cross-vendor GPU float determinism is impractical; the GPU path would be
  an **opt-in, non-deterministic** fast path for effects/non-gameplay bodies, while gameplay-
  critical deterministic simulation stays on the CPU path. Call this out explicitly so it doesn't
  silently break the lockstep guarantee.

---

## Cross-cutting: determinism strategy (hard requirement)

- **Compiler/float discipline** (project-wide): `-ffp-contract=off` (and `/fp:precise` on MSVC),
  never `-ffast-math`; force SSE2+ (no x87) on 32-bit; single-precision `f32` throughout (already
  the convention). Verify GCC/Clang/MSVC agree via the Phase-0 determinism test in CI.
- **Engine-owned transcendentals.** `std::sin/cos/acos/sqrt` differ across libm implementations;
  route solver/quaternion math through a small fixed-implementation math lib (or restrict to the
  ops that are IEEE-correctly-rounded, i.e. `sqrt`) for anything that affects simulation state.
- **Stable ordering, never address-based.** All sorts use `(entityId, entityId, featureId)` keys;
  no ordering derived from pointer values, hash-map iteration, or thread completion order.
- **Fixed partitioning** in `parallel_for` and **worker-order merges** for all result buffers.
- **Deterministic warm-start** via stable contact feature IDs.
- The determinism test (Phase 0) runs single-threaded vs multi-threaded and 2× repeat in CI as a
  hard gate.

## Additional recommendations (beyond raw speed)

- **Thread-safe scene queries.** `TestOverlap`/`TestCollision`/raycasts should be safe to call
  from worker threads (needed once systems parallelize); the per-thread tree scratch from Phase 3
  enables this.
- **Decouple runtime `RigidBody` objects from the hot loop.** `CreateRigidBody` heap-allocates a
  `RigidBody` and stores raw pointers (`physics_world.cpp:112`); keep those as the *handle/API*
  layer but ensure the step loop touches only SoA arrays (mostly true already — finish it).
- **Batch create/destroy** to avoid mid-frame store reshuffles; defer structural changes to step
  boundaries.
- **Event/callback dispatch** (`ReportContactsAndTriggers`) should collect events into per-worker
  buffers during parallel narrowphase and fire them serially in a deterministic order.
- Keep the **public API and Doxygen-commented style** (`CLAUDE.md` conventions) stable so this is
  an internal re-architecture, not an API break, wherever possible.

---

## Verification strategy

Every phase is gated by the existing Catch2 suite plus the new harness:

- **Correctness:** `/test clang-all-debug` (mirrors `tests/src/physics/`) stays green after each
  phase; extend coverage for the new job system, coloring, and TGS solver.
- **Determinism:** the Phase-0 hash test must pass **after every phase**, including
  single-thread-vs-multi-thread equality once Phase 2+ lands. This is the primary guard for the
  hard determinism requirement.
- **Performance:** the Phase-0 benchmark scenes produce per-stage timings; record a baseline now
  and track speedup per phase (target: near-linear scaling of solve+collision with core count on
  the large scenes, plus the SIMD multiplier on the solver).
- **End-to-end:** run the sandbox (`build/<preset>/examples/sandbox/sandbox`) with a heavy scene
  to confirm visual stability (no jitter/explosions) under the new substepped solver.
- Use `/build clang-all-debug` for iteration and a release preset for benchmark numbers.

## Suggested execution order

Phase 0 → 1 (ship value + determinism foundation with zero threading risk) → 2 (job infra) →
3 (parallel collision) → 4 (modern parallel solver — the headline change) → 5 (SIMD + memory) →
6 (CCD) → 7 remains design-only. Phases 3/4/5 are where the multi-core + SIMD performance
actually lands; 0/1 de-risk everything that follows.
