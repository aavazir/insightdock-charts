#!/bin/sh
# Fails if the version declared in CMake, the OpenWrt package and the vcpkg port disagree.
# Pre-release suffixes (e.g. 1.0.0-rc1) are ignored: CMake project() can't carry them.
set -eu
cd "$(dirname "$0")/.."

cmake_v=$(sed -n 's/^project([A-Za-z]* VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
owrt_v=$(sed -n 's/^PKG_VERSION:=\(.*\)/\1/p' packaging/openwrt/Makefile | sed 's/-.*//')
vcpkg_v=$(sed -n 's/.*"version\(-string\|-semver\)\{0,1\}": *"\([^"]*\)".*/\2/p' \
  packaging/vcpkg/insightdock-charts/vcpkg.json | sed 's/-.*//')

echo "CMake=$cmake_v OpenWrt=$owrt_v vcpkg=$vcpkg_v"
[ -n "$cmake_v" ] && [ "$cmake_v" = "$owrt_v" ] && [ "$cmake_v" = "$vcpkg_v" ] || {
  echo "version mismatch" >&2
  exit 1
}
