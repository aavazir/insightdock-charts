# Release process

## Versioning

Semantic versioning after 1.0.0, promising **source compatibility** only: public option structs may
gain fields in minor releases, so binary (ABI) compatibility is not guaranteed. `SOVERSION` is
always `major.minor`; patch releases must preserve ABI within that minor; `find_package` uses `SameMinorVersion`
before 1.0. See `docs-site/guides/thread-safety-and-abi.md`.

## Release checklist

M1 release readiness is tracked in [the completion issue](https://github.com/aavazir/insightdock-charts/issues/5)
and [the delivery plan](planning/M1.md). The checklist below applies to a specific candidate;
previous local validation is not a substitute for testing that revision. Record the candidate
commit, remote CI URLs, archive checksums, clean-consumer logs and publication links in
[the final acceptance issue](https://github.com/aavazir/insightdock-charts/issues/19).
All original M1 work must be completed or explicitly rescoped by a recorded maintainer decision.
Use a feature freeze for the release candidate: new chart-family work stays in later trackers.

- [ ] Clean clone builds on Linux (`cmake` + `make`)
- [ ] `ctest` passes
- [ ] Generic examples produce PNG/PDF outputs
- [ ] `pie_doughnut_example` produces PNG/native SVG/native PDF with matching colors and labels;
      check SVG/PDF with independent renderers and extract PDF text
- [ ] `cmake --install` + consumer `find_package(InsightDock)` works
- [ ] No customer/internal references remain
- [ ] `CHANGELOG.md` updated
- [ ] `NOTICE` and font licenses reviewed
- [ ] GitHub Actions CI green on `main`, including the PDF-enabled job
- [ ] Formatting, timezone regressions, sanitizer checks and installed consumers pass
- [ ] Compatibility/default changes documented with upgrade guidance
- [ ] Third-party license files included in installation and release artifacts
- [ ] Security policy/support status and roadmap updated with verified outcomes
- [ ] GitHub Pages site + gallery published
- [ ] vcpkg port (optional): `insightdock-charts`
- [ ] Tag: `vX.Y.Z`

## Release candidates

Publish `vX.Y.Z-rcN` first. `scripts/check_version.sh` strips the `-rcN` suffix, so CMake, OpenWrt and vcpkg keep the plain `X.Y.Z`. After the feedback window, tag the final release. Fill the vcpkg portfile `SHA512` from the tag's source archive before submitting the port.

## Tagging

```bash
git tag -a v1.0.0-rc1 -m "InsightDock Charts for C++ v1.0.0-rc1"
git push origin v1.0.0-rc1
```

## Artifacts

For releases, attach:

- Source tarball or GitHub release archive
- Example PNG outputs from `bar_line_example`
- Optional PDF from `podofo_report_example` when PoDoFo is available
