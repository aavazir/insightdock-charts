# Contributing to InsightDock Charting Library for C++

Thank you for improving InsightDock.

## Getting started

1. Fork and clone the repository.
2. Build the core library:
   ```bash
   cmake -B build -DINSIGHTDOCK_BUILD_EXAMPLES=ON -DINSIGHTDOCK_BUILD_TESTS=ON
   cmake --build build
   ctest --test-dir build
   ```
3. For PDF features, enable PoDoFo:
   ```bash
   cmake -B build-podofo -DINSIGHTDOCK_WITH_PODOFO=ON -DINSIGHTDOCK_PODOFO_DIR=/path/to/podofo
   cmake --build build-podofo
   ```

## Code style

- C++17, STL-first core (no GUI dependencies).
- **CI uses clang-format 18** — run the project checker before every commit:
  ```bash
  ./scripts/check-format.sh --fix   # apply formatting
  ./scripts/check-format.sh         # verify (same as CI lint job)
  ```
  Enable the git hook once per clone: `git config core.hooksPath .githooks`
- Style rules: `.clang-format` (LLVM, 100-column limit).
- Keep the public API simple: defaults should produce good charts without extra configuration.
- Add or update examples and docs for every public API change.

## Pull requests and CI

This project uses **GitHub Actions** (`.github/workflows/ci.yml`).

1. Branch from `main`: `git checkout -b feature/short-description`
2. Push early and open a **draft pull request** — CI runs on every push.
3. Before marking ready: local build passes, `CHANGELOG.md` updated, `clang-format` clean.

**CI jobs:**

| Job | What it checks |
|-----|----------------|
| `lint` | `clang-format --dry-run --Werror` on `src/`, `include/`, `tests/` |
| `lint-tidy` | `clang-tidy` on `src/*.cc` (optional until promoted) |
| `build-and-test` | CMake configure, build, and `ctest` |
| `sanitizers` | ASan + UBSan build and tests |

## Pull request checklist

- One logical change per pull request when possible.
- Update `CHANGELOG.md` for user-visible changes.
- Ensure generic examples build; run tests when available.
- Do not add customer-specific references to the core library.

## Extensibility

New chart types should plug into `Chart` / `CartesianChart` and work with `ChartExporter`. See `ROADMAP.md` for planned areas and `docs/API.md` for the public surface.

## Questions

Open a GitHub issue or start a discussion in the repository.
