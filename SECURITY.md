# Security policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 1.0.x (including release candidates) | Yes |
| < 1.0 | No |

Supported versions receive security fixes. Applications must rebuild when upgrading between minor
releases because the library promises source compatibility, not cross-minor binary compatibility.

## Reporting a vulnerability

Use [GitHub private vulnerability reporting](https://github.com/aavazir/insightdock-charts/security/advisories/new).
Include the affected revision, compiler/platform, enabled features, minimal reproducer and observed
impact. Avoid public issues for undisclosed vulnerabilities and omit credentials or private data.

Maintainers will acknowledge reports as soon as practical, investigate supported versions, and
coordinate a fix and disclosure with the reporter. This volunteer project does not promise a
fixed response time. If GitHub private reporting is unavailable, contact the repository owner
through the contact information on their public profile to establish a private channel.

## Input and trust boundaries

- The API consumes chart values, labels, options and optional font data supplied by application
  code. The core has no JSON parser or embedded-code execution API.
- Applications should limit point counts, label lengths, concurrent jobs and requested dimensions
  before rendering externally supplied data. The image/export pixel cap is an allocation guard,
  not a complete process memory budget; supersampling and output encoding allocate additional memory.
- Use trusted font files. The font APIs check size and basic parseability, but the vendored
  `stb_truetype` parser is not a hardened validator or sandbox for attacker-controlled TTF files.
  Do not pass uploaded/untrusted fonts to these APIs in a privileged process.
- Treat export paths as application-controlled. Validate destinations and permissions before
  accepting user-directed writes. The library does not confine paths to a directory.
- SVG text is escaped, but applications serving SVG should still choose suitable content type and
  origin/isolation policies for their deployment.
- Keep the optional PoDoFo backend and its dependencies updated. Core CI runs ASan/UBSan and
  hostile numeric-input regressions; the PDF job exercises the pinned optional backend.

## Dependency changes

Preserve upstream notices and document source revisions for vendored updates. Review security
advisories and run affected sanitizer, renderer and consumer checks before releasing. Dependency
and release responsibilities are described in [GOVERNANCE.md](GOVERNANCE.md).
