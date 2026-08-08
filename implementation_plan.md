# X1-BOX — Implementation & Optimization Plan

## Status

| Status | Commit | Description |
|--------|--------|-------------|
| ✅ Done | `e851e0bcf` | Code-review fixes: version bump, Vulkan surface bugs, mutex, dead code |

---

## Priority 1 — Critical Bugs (Must Fix)

### 1.1 Missing Surface Downloads (Vulkan Black Textures)
**Symptom:** Textures appear as black/empty on some games.
**Root cause (from issue tracker):** The `pgraph_vk_download_surfaces_in_range_if_dirty` function drops partial downloads when the deferred-download array overflows.
**Status:** ✅ **Fixed** in `e851e0bcf` — overflow now flushes prior batch instead of silent-drop.
**Verification:** Test with games that use large textures (e.g. *Hollow Knight*, *Celeste*).

### 1.2 Global Error Flag Never Resets (`g_surface_addr_map_missing_logged`)
**Symptom:** After one surface address lookup failure, all subsequent failures are silently suppressed — no logging ever again.
**Status:** ✅ **Fixed** in `e851e0bcf` — flag now resets on renderer init **and** finalize.
**Verification:** Force a bad address lookup twice and check for duplicate log messages.

### 1.3 Partial + Downscale Region Mismatch
**Symptom:** When downscaling a partial surface download (e.g. scrolling viewport), the blit region is not rescaled — the entire scaled surface is blit-ed instead of just the partial row range.
**Status:** ✅ **Fixed** in `e851e0bcf` — row offsets now rescaled to match the scaled dimensions.
**Verification:** Test scrolling games with scaling enabled (e.g. *Super Meat Boy*).

### 1.4 Deferred Download Race Condition
**Symptom:** Under concurrent downloads, `deferred_downloads_frame` checks can race, causing stale frame numbers to trigger spurious flushes.
**Status:** ✅ **Fixed** in `e851e0bcf` — check is now under `qemu_mutex_lock(&d->pgraph.lock)`.
**Verification:** Stress-test with multiple rapid surface switches.

---

## Priority 2 — Performance Optimization

### 2.1 Missing `-O3` in Android Release Builds
**Current:** `build.gradle.kts` uses `-O2 -g0` for Debug, Release, and RelWithDebInfo.
**Fix:** Add `-O3` to Release builds for maximum GPU emulation throughput.
**Risk:** Low — already verified on Windows builds; `-ffast-math` removed from the path.
**Target commit:** `v3.9.2` — include in the next build.

### 2.2 `-march=native` Limits Portability
**Current:** `cmake/EmulationOptions/CMakeLists.txt` uses `-march=native` which hard-codes host CPU features (e.g. AVX2 on x86).
**Issue:** Binaries won't run on devices without the same instruction set.
**Fix:** Replace with `-march=armv8-a` for Android targets; keep `-march=native` only for Windows dev builds.
**Target commit:** `v3.9.2` — conditional on `OS=Android`.

### 2.3 TCG Peephole Optimizations — SVE2 Fast-Path
**Current:** `tcg/aarch64/` already has SVE2 fast-path for 128-bit vector math.
**Status:** ✅ Already present (`da3f4356a8`). No further action.

### 2.4 LSE Atomics Integration
**Current:** `8cc640a8d4` implements ARMv9 LSE atomics.
**Status:** ✅ Already present. No further action.

### 2.5 `XEMU_ENABLE_LTO` Not Passed in Android
**Current:** `build.gradle.kts` defines `XEMU_ENABLE_LTO=ON` for Release but never passes it to `cmake`.
**Fix:** Add `-DXEMU_ENABLE_LTO=ON` to the Release `cmake` arguments block.
**Target commit:** `v3.9.2` — add in same commit as `-O3` flag.

---

## Priority 3 — Missing / In-Progress Fixes

### 3.1 `nv2a.c` — Consecutive Tile Blit TODO
**Location:** `hw/xbox/nv2a/nv2a.c`, lines ~102-103
**Issue:** When a blit crosses two GPU tiles that are consecutive in the tile table, the hardware behavior is not deterministic. The current code returns `limit + 1 - blit_base_address` as a fallback, but this may be wrong.
**Required action:** 
  1. Research the actual HW behavior via NVIDIA documentation / reverse-engineering.
  2. Implement the correct address calculation.
  3. Add a test case for the edge case.
**Status:** ❌ Open — requires more investigation.

### 3.2 Sync Vulkan Backend with hakuX Fork
**Reference:** https://github.com/hakuX/X1-BOX
**Delta:** ~20,000 lines of changes in `hw/xbox/nv2a/pgraph/vk/`
**Key changes to integrate:**
  - `vulkan.c` — Major refactor of the Vulkan command buffer flow
  - `surface.c` — More efficient surface flushing
  - `pgraph.c/h` — Additional Vulkan caching hooks
  - `display.c` — New rendering paths
  - `vsh.c` — Vertex shader optimizations
  - `pgraph.c` — Synchronization improvements
**Target commit:** `v3.9.2` (if we can merge cleanly) or `v3.10.0` (if a branch split is needed)
**Status:** ❌ Open — needs full diff analysis and merge testing.

### 3.3 `nv2a.c` — Texture Upload Path (CPU Conversions)
**Location:** `hw/xbox/nv2a/texture.c`
**Issue:** Texture uploads currently go through unnecessary CPU-side pixel-format conversions.
**Fix:** Identify where conversions are redundant (e.g. if GPU input matches Vulkan surface format) and skip them.
**Target commit:** `v3.9.2` — can be done as part of the hakuX sync.
**Status:** ❌ Open — depends on hakuX sync.

### 3.4 `nv2a.c` — VBlank "Unlock Mode" Hysteresis Tuning
**Current:** `enter_thresh = period + period/2`, `exit_thresh = period * 2 + period/2`
**Issue:** The hysteresis window may be too wide for 30fps games that occasionally miss their deadline.
**Fix:** Add a per-game override in `config.json` (if present) to let the user tune this.
**Status:** ❌ Open — depends on whether a config file mechanism exists.

---

## Priority 4 — Cleanup & Maintainability

### 4.1 `PGRAPHState` — Unused `enable_vertex_program_write`
**Location:** `hw/xbox/nv2a/pgraph/pgraph.h`, line ~183
**Status:** ✅ **Removed** in `e851e0bcf`. Saves 4 bytes per instance (trivial, but good practice).

### 4.2 `pgraph.h` — Stale FIXME on `surface_binding_dim`
**Location:** `pgraph.h`, line ~160
**Note:** Comment says `/* FIXME: Refactor */` but no action item is clear.
**Status:** ⚪ Pending — leave as-is unless the struct is refactored elsewhere.

### 4.3 `pgraph.h` — Stale FIXME on `pgraph_is_texture_sampler_active()`
**Location:** `pgraph.h`, line ~418
**Note:** Comment says `// FIXME: Add new function pgraph_is_texture_sampler_active()` but it already exists (as an inline function).
**Status:** ⚪ Pending — remove the FIXME comment in the next refactor cycle.

---

## Priority 5 — Build Infrastructure

### 5.1 Android `proguard-rules.pro` Missing Native Exports
**Current:** Debug symbols are kept for `.so` files but no proguard rules protect native exports.
**Fix:** Add `keep class com.xbox.nv2a.* { *; }` to `proguard-rules.pro`.
**Target commit:** `v3.9.2` — safety measure for Release builds.
**Status:** ⚪ Open.

### 5.2 Version Bump Protocol
**Current:** `build.gradle.kts` is edited manually.
**Fix:** Create a release script (`upload_apk.py`) that:
  1. Reads the version from the git tag.
  2. Updates `versionName` and `versionCode`.
  3. Runs the build and generates the APK.
  4. Uploads to GitHub Releases.
**Status:** ⚪ Open — implement before `v3.9.2`.

### 5.3 `3.x.x` Branch Alignment
**Current:** The branch was incorrectly diverged to `v3.0.0` and has been rolled back to `v3.9.1`.
**Status:** ✅ **Aligned** to `v3.9.1` in previous commit.
**Next release:** `v3.9.2`

---

## Next Release: `v3.9.2`

**Goal:** Ship the first 3.9.x release with all critical bugs fixed.
**Required commits:**

| # | Fix | Estimated Effort |
|---|-----|-----------------|
| A | `-O3` flag in Release + LTO pass-through | 5 min |
| B | `proguard-rules.pro` for native exports | 10 min |
| C | Release build & test | 30 min |
| D | Create `upload_apk.py` automation | 2 hours |
| E | Tag + release | 10 min |

**Total: ~2.5 hours** (assuming the release build works).

---

## After v3.9.2: `v3.9.3`

**Goal:** Integrate the hakuX Vulkan backend sync.
**Risk:** Medium — this is a large diff and may require merge resolution.
**Approach:** 
  1. Create a `vulkan-sync` feature branch.
  2. Cherry-pick individual hakuX commits one at a time, testing after each.
  3. Merge when stable.

---

## After v3.9.3: `v3.10.0`

**Goal:** Major feature improvements.
**Candidates:**
- Texture upload CPU conversion elimination
- VBlank unlock mode per-game tuning
- Consecutive tile blit fix
- Any remaining pgraph.h FIXME cleanup

---

## Known blockers

1. **GitHub CLI auth:** The stored `ghp_...` token is not propagating in this terminal environment. Use `GH_TOKEN=ghp_... gh release create ...` instead, or log in via browser.
2. **Gradle build cache:** The terminal environment lacks a warm Gradle cache. Builds will be slow. Use `bash android/gradlew assembleDebug` (not `./gradlew`).
3. **`debugkeystore` limitation:** Debug APKs are signed with a per-machine keystore and can only be installed on the machine that built them. Use `assembleRelease` with a custom keystore for distributable builds.
