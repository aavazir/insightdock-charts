# Project governance

InsightDock Charts is maintained through public issues and reviewed pull requests. The repository owner
and maintainers with merge access are responsible for releases, compatibility decisions and
security fixes. Current maintainer access is managed in GitHub; this document does not grant it.

## Decisions and contributions

- Bug fixes and documentation improvements can be proposed directly as pull requests.
- Discuss new public APIs, dependencies, chart families and compatibility changes in an issue
  before implementing them. Include the user workflow, examples and alternatives.
- Maintainers prioritize the milestones in [ROADMAP.md](ROADMAP.md), reproducible bugs, and
  demonstrated adopter needs. A roadmap entry is not a release date commitment.
- M1 is the active delivery target; later milestones remain planned backlog. Use the
  [M1 completion tracker](https://github.com/aavazir/insightdock-charts/issues/5) for live work
  status and [the delivery plan](docs/planning/M1.md) for acceptance and release evidence.
  Scope changes require an explicit recorded decision; do not silently defer an M1 gate.
- Prefer small changes that can be reviewed independently. Public APIs need user documentation,
  error/missing-value semantics, and meaningful regression coverage.
- The author addresses review feedback; a maintainer confirms required CI checks and release
  impact before merging. If a maintainer authors a change, seek another review when available.
- Contributions use the existing project license. Changes incorporating third-party material
  must preserve its license notices and explain the source.

## Releases and maintenance

Follow [docs/RELEASE.md](docs/RELEASE.md). Record user-visible changes in `CHANGELOG.md`; publish
upgrade guidance when defaults, output or binary compatibility change. Supported versions and
private vulnerability reporting are defined in [SECURITY.md](SECURITY.md).

Keep general project discussion public and searchable. Security reports use the private reporting
channel. Conduct reports follow [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md). Maintainers explain
technical decisions in the relevant issue or pull request and may revisit them with new evidence.

## Dependencies

The core remains C++17 with vendored stb and embedded font subsets. Optional dependencies require
a documented feature/build boundary. Before updating vendored code or CI tooling, record the
upstream version or immutable revision, preserve licenses, inspect the change and run affected
checks. Automated dependency updates receive the same review as other contributions.
