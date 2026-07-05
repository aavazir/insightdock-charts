# Troubleshooting

Answers to the problems that actually come up when building, installing, or extending
InsightDock — as opposed to a hypothetical FAQ. Each entry below was a real incident.

## `bad_alloc` / `length_error` from `ChartExporter::save()` after a routine rebuild

**Symptom:** intermittent `std::bad_alloc` or `std::length_error` thrown from inside
`ChartExporter::save()` or `render()`, with no change to your own code — often right after a
`git pull` or after re-running `cmake --install` against an old build directory.

**Cause:** this is an ABI mismatch, not a library bug. `install(DIRECTORY include/insightdock ...)`
copies headers straight from the live source tree regardless of whether the compiled library in
your build directory is current. If a public struct passed by value — `ExportOptions` is the one
that has bitten this before, when it grew from 16 to 20 bytes adding `svg_backend` — changed size
between the two, every call through the mismatched boundary reads garbage from the wrong
registers/stack slots. No compiler warning, no link error: on x86-64 SysV, structs ≤16 bytes pass
in registers and >16 bytes pass on the stack, so a caller built against the newer header and an old
precompiled library disagree about *how the struct arrives*. When the garbage value lands in
`ExportOptions::supersample` and is `>1`, `SupersampleScope` multiplies width/height by it and the
backing allocation blows up — hence `bad_alloc`.

**Fix:** always run `cmake --build <dir>` immediately before `cmake --install <dir>`. Re-running
just the install step after a `git pull`, or testing against a long-lived build directory you
forgot to rebuild, are the two ways this bites. See the "Installing for out-of-tree consumers" note
in `CLAUDE.md` and the `[Unreleased]` → `Fixed` entry in `CHANGELOG.md` for the full root-cause
writeup (this was tracked down with a GDB hardware watchpoint, since ASan's own allocator behavior
and GDB's default ASLR-disabling both hid the symptom).

**If you hit something similar:** any public struct passed by value across the library boundary is
a potential instance of this class of bug if its size changes between header and binary. If a
report of "intermittent crash after an unrelated change" doesn't reproduce under a debugger, suspect
a stale install before suspecting a data race.

## clangd shows `file not found` or `use of undeclared identifier 'std'`

These are false positives from an unconfigured compilation database, not real build failures.
Generate one and reload the editor:

```bash
./scripts/setup_clangd.sh
```

Then reload the IDE window (Cursor: Command Palette → "Developer: Reload Window"). This produces
`build/compile_commands.json` (core + PoDoFo sources when available) and pairs with the
repo's `.clangd` config file. See `docs/DOCUMENTATION.md` for details.

## `cmake -DINSIGHTDOCK_WITH_PODOFO=ON` fails with a CMake version error

PoDoFo requires **CMake 3.23+**. If your system CMake is older:

```bash
pip install --user 'cmake>=3.23'
export PATH="$HOME/.local/bin:$PATH"
```

## MrDocs / API reference generation fails on an aarch64 CI runner

`mrdocs` ships x86_64-only Linux binaries. Running the binary directly on an aarch64 host fails
because its `compile_commands.json` points at aarch64 system headers (ARM SVE types) that an
x86_64-targeted clang can't parse. The fix is architectural, not a flag: run the whole
configure+build+mrdocs step inside an amd64 container under QEMU user-mode emulation
(`scripts/ci_docs_mrdocs.sh`), so headers and the parsing clang stay architecture-consistent. This
needs the binfmt handler registered once per runner boot:

```bash
docker run --privileged --rm tonistiigi/binfmt --install amd64
```

Not persisted across reboots yet — `apt install qemu-user-static binfmt-support` (needs root) would
make it permanent. See `docs/DOCUMENTATION.md`.

## Updating the visual regression baseline

`gallery_regression` (run via CTest) compares each chart type's rendered PNG against a committed
reference in `docs/gallery/`, tolerant of up to 0.1% differing pixels (font rasterization can vary
very slightly across platforms). A failing `gallery_regression` run means one of two things:

- **An unintended regression** — something changed the rendered output you didn't mean to change.
  Investigate before touching the baseline.
- **An intentional visual change** — a deliberate styling/layout change to that chart type. Update
  the baseline:

```bash
cmake --build build --target insightdock_gallery_regression
UPDATE_GALLERY=1 ./build/tests/insightdock_gallery_regression docs/gallery
```

Review the diff on the regenerated PNGs before committing — `git diff --stat docs/gallery/` won't
show you *what* changed, only that it did.

## Cross-compiling for OpenWrt fails to link

The plain `Makefile` build (not CMake) is the supported OpenWrt path — it has zero link-time
external dependencies in the core. Confirm the toolchain prefix matches your target
(`mips-openwrt-linux-musl-g++`/`-gcc-ar`, or your SDK's equivalents) and that `CHART_TYPES` /
`EXPORT_FORMATS` / `FONT_MODE` are set consistently between the library build and any example you
link against — a mismatch (e.g. building the library with `CHART_TYPES="pie"` but linking an
example that calls `BarChart`) fails at link time with undefined-symbol errors, not a clear
"chart type not compiled" message. See [Getting started](../getting-started.md) for the full
variable table.
