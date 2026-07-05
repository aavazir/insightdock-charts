# Future developer experience

The MkDocs Material site + embedded MrDocs API reference described below is
now **built** (`mkdocs.yml`, `docs-site/`, `scripts/build_docs_site.sh` — see
[DOCUMENTATION.md](DOCUMENTATION.md#build-the-full-site-mkdocs-material--embedded-api-reference))
and published to GitHub Pages via `.github/workflows/docs.yml`. Everything
below this point (comparison hub tabs/use-case picker, code generator,
SKILLS.md, agent workflow docs) is still aspirational, post-v1.0.0.

## Public website (GitHub Pages)

Site layout (built):

- `/` — landing, quick start ([index.md](../docs-site/index.md))
- `/getting-started/` — build instructions ([getting-started.md](../docs-site/getting-started.md))
- `/examples/` — source + rendered output per example ([examples.md](../docs-site/examples.md))
- `/api/` — MrDocs reference (copied from `docs/reference/` by the build script)
- `/comparison/` — proprietary + open source (transcludes [COMPARISON.md](COMPARISON.md))

### Comparison hub (not yet built)

- Tabs: Commercial | Open source (C++) | Web / other
- Feature matrix: license, headless, PDF vector, embedded, chart breadth, finance
- Use-case picker with honest recommendations

## Online code generator

Interactive page (MkDocs plugin or static JS):

1. User selects chart type (pie, bar, line)
2. Enters sample labels/values and export format
3. Receives copy-paste C++ skeleton using `insightdock/charts.h`

## SKILLS.md and agent support

Document patterns for AI agents and IDE skills:

- Generate a pie chart from JSON values
- Build a PDF report section with vector pie
- Map analytics payload fields to chart configuration

Example skill prompt:

> Create an InsightDock pie chart from `{ "Online": 72, "Offline": 28 }` and save as PNG using `ChartExporter::save`.

## Agent workflow targets

- `ChartExporter::save` one-liner examples in docs
- JSON → chart config mapping for report payloads
- Deterministic output for visual regression in CI
