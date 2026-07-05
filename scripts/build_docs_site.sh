#!/usr/bin/env bash
# Assembles the MkDocs site (mkdocs.yml, docs-site/) from existing
# sources-of-truth — docs/gallery/ (chart images) and docs/reference/ (the
# MrDocs API reference) — then builds the static site to site/.
#
# Prerequisites:
#   python3 -m venv .venv-docs && .venv-docs/bin/pip install mkdocs-material
#   docs/reference/ already generated (see docs/DOCUMENTATION.md)
set -euo pipefail
cd "$(dirname "$0")/.."

if [ ! -d docs/reference ]; then
  echo "docs/reference/ not found — generate the MrDocs API reference first" >&2
  echo "(see docs/DOCUMENTATION.md, 'Generate API reference')." >&2
  exit 1
fi

mkdir -p docs-site/img/gallery
cp docs/gallery/*.png docs-site/img/gallery/

rm -rf docs-site/api
cp -r docs/reference docs-site/api

MKDOCS="${MKDOCS:-.venv-docs/bin/mkdocs}"
"$MKDOCS" build "$@"
