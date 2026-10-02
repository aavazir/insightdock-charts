# Release process

## Versioning

Semantic versioning after 1.0.0, promising **source compatibility** only: public option structs may
gain fields in minor releases, so binary (ABI) compatibility is not guaranteed. `SOVERSION` is
`major.minor` while the major is 0 and `major` from 1.0; `find_package` uses `SameMinorVersion`
before 1.0. See `docs-site/guides/thread-safety-and-abi.md`.

## Release checklist

- [ ] Clean clone builds on Linux (`cmake` + `make`)
- [ ] `ctest` passes
- [ ] Generic examples produce PNG/PDF outputs
- [ ] `cmake --install` + consumer `find_package(InsightDock)` works
- [ ] No customer/internal references remain
- [ ] `CHANGELOG.md` updated
- [ ] `NOTICE` and font licenses reviewed
- [ ] GitHub Actions CI green on `main`
- [ ] GitHub Pages site + gallery published
- [ ] vcpkg port (optional): `insightdock-charts`
- [ ] Tag: `vX.Y.Z`

## Tagging

```bash
git tag -a v1.0.0 -m "InsightDock Charts for C++ v1.0.0"
git push origin v1.0.0
```

## Artifacts

For releases, attach:

- Source tarball or GitHub release archive
- Example PNG outputs from `bar_line_example`
- Optional PDF from `podofo_report_example` when PoDoFo is available
