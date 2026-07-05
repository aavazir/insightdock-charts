# Release process

## Versioning

Semantic versioning after 1.0.0.

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
- [ ] Brand clearance complete ([BRAND_CLEARANCE.md](BRAND_CLEARANCE.md))
- [ ] vcpkg port (optional): `insightdock-charting`
- [ ] Tag: `vX.Y.Z`

## Tagging

```bash
git tag -a v1.0.0 -m "InsightDock Charting Library for C++ v1.0.0"
git push origin v1.0.0
```

## Artifacts

For releases, attach:

- Source tarball or GitHub release archive
- Example PNG outputs from `bar_line_example`
- Optional PDF from `podofo_report_example` when PoDoFo is available
