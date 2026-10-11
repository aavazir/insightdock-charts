# Contributing to InsightDock Charts for C++

See [GOVERNANCE.md](GOVERNANCE.md) for maintainer responsibilities and API/dependency decisions,
[ROADMAP.md](ROADMAP.md) for priorities, and [SECURITY.md](SECURITY.md) for private reporting.

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

Use **InsightDock Charts for C++** as the full project name and **InsightDock Charts** as the
short name in documentation, headings, descriptions and image alt text. Keep the capital D and
do not shorten the public name to InsightDock alone. Existing technical identifiers, namespaces,
package names and paths retain their spelling.

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

Current work targets [M1 completion](https://github.com/aavazir/insightdock-charts/issues/5).
Choose a focused linked issue, confirm its dependencies and acceptance criteria, and reference it
in the PR. See [the delivery plan](docs/planning/M1.md). M2–M4 are deferred planning trackers;
discuss promotion before beginning substantial new feature work. Close an implementation issue
after its code/docs are merged and relevant evidence is linked, rather than after a local build.

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
| `pdf` | Pinned PoDoFo build, PDF tests/report examples and installed PDF consumer |
| `make-build` | Default and minimal embedded build configurations |
| `version-consistency` | Package versions agree |

## Pull request checklist

- One logical change per pull request when possible.
- Update `CHANGELOG.md` for user-visible changes.
- Ensure generic examples build; run tests when available.
- Do not add customer-specific references to the core library.
- Include focused regression cases for behavior or reliability fixes; avoid tests that merely repeat implementation details.
- Document public API defaults, missing-value behavior, errors, backend limitations and compatibility impact.
- Preserve license notices and record upstream revisions for new or updated third-party material.
- Keep external inputs bounded; treat font files and filesystem paths according to `SECURITY.md`.

## Extensibility

New chart types should plug into `Chart` / `CartesianChart` and work with `ChartExporter`. See `ROADMAP.md` for planned areas and `docs/API.md` for the public surface.

## Questions

Open a GitHub issue or start a discussion in the repository.
