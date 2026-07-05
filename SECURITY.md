# Security Policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 0.x (pre-1.0) | Yes |
| < 0.6   | No |

## Reporting a vulnerability

If you discover a security issue, please report it privately via [GitHub private security advisories](../../security/advisories/new) rather than a public issue, or contact the maintainers listed in the repository.

Do not open public issues for undisclosed vulnerabilities.

## Scope

InsightDock renders untrusted chart data and optional JSON payloads passed by application code. Treat external input as untrusted:

- Validate JSON schema and size limits in application code.
- Do not execute embedded content from chart payloads.
- Keep PoDoFo and other optional dependencies updated.

We will acknowledge reports within a reasonable timeframe and coordinate fixes before public disclosure when applicable.
