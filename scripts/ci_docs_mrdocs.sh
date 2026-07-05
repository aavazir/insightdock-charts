#!/usr/bin/env bash
# Builds the core library and generates the MrDocs API reference (docs/reference/).
# See the docs job in .github/workflows/docs.yml. mrdocs only ships x86_64 Linux
# binaries, so this must run on an x86_64 Linux host (GitHub's ubuntu-latest
# runners qualify natively — no container/emulation needed).
set -euo pipefail

WORKDIR="$PWD"
MRDOCS_VERSION="${MRDOCS_VERSION:-0.8.0}"

sudo apt-get update -qq
sudo apt-get install -y -qq build-essential cmake curl >/dev/null

MRDOCS_BIN="$PWD/.mrdocs-tool/MrDocs-${MRDOCS_VERSION}-Linux/bin"
if [ ! -x "$MRDOCS_BIN/mrdocs" ]; then
  rm -rf .mrdocs-tool && mkdir -p .mrdocs-tool
  curl -fsSL "https://github.com/cppalliance/mrdocs/releases/download/v${MRDOCS_VERSION}/MrDocs-${MRDOCS_VERSION}-Linux.tar.gz" \
    -o /tmp/mrdocs.tar.gz
  tar -xzf /tmp/mrdocs.tar.gz -C .mrdocs-tool
fi
export PATH="$MRDOCS_BIN:$PATH"

# Stock mrdocs 0.8.0 ships zero CSS — raw browser-default white-on-black
# text. --addons requires a full replacement of its internal template tree
# (helpers + partials + layouts), too heavy to vendor just to fix styling,
# so instead overwrite its one wrapper layout (every run, not just on first
# extraction, so theme edits take effect even when .mrdocs-tool/ is cached).
cp "$WORKDIR/docs/mrdocs-theme/wrapper.html.hbs" \
  "$MRDOCS_BIN/../share/mrdocs/addons/generator/html/layouts/wrapper.html.hbs"

mrdocs --version
cmake -B build \
  -DINSIGHTDOCK_BUILD_EXAMPLES=ON \
  -DINSIGHTDOCK_BUILD_TESTS=OFF \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j"$(nproc)" --target InsightDockCharting
cd docs && mrdocs mrdocs.yml
test -f reference/index.html
