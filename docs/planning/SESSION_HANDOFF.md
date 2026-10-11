# Session handoff — 2026-10-11

Project name: **InsightDock Charts for C++**; short name: **InsightDock Charts**.

## Published work

[PR #24](https://github.com/aavazir/insightdock-charts/pull/24) contains the completed rendering,
export-contract, installation, licensing, documentation and planning work. Branch:
`review/m1-reliability-and-project-plan`. Dependency PRs #1–#4 are merged into `main` and
integrated into this branch, including action upgrades for the new PDF job.

The PR is open for review. Check its latest GitHub Actions results before merging; successful
local checks do not assert remote CI success or release readiness. Documentation deployment
runs after merge to `main`; this session has not published a release.

## Verification

After integrating `main`, all eight core CTest checks pass, as do clang-format-18, version
consistency, whitespace checks and strict MkDocs. Prior review validation covered PDF-enabled
and shared export/installed-consumer checks and GCC ASan/UBSan; see ROADMAP.md and the PR.
The local docs build uses the existing generated API reference. Fresh MrDocs generation and
remote platform results still need their own evidence.

## Next session

1. Review PR #24 and its checks; resolve any failures before merge.
2. Continue M1 with [font fidelity and missing-glyph diagnostics #6](https://github.com/aavazir/insightdock-charts/issues/6).
3. Follow [M1.md](M1.md) for validation/budgets and remaining acceptance work. M1 parent
   [#5](https://github.com/aavazir/insightdock-charts/issues/5) remains open.
4. M2–M4 and specialized work stay deferred in issues #20–#23. Repository milestone/Project
   administration remains planned in M1.md.

Keep this branch until the PR is merged. Do not tag a release or close M1 based on this session.
