# OpenWrt packaging sample

`Makefile` here is a standard OpenWrt feed package definition (the same shape as any
[package/libs](https://github.com/openwrt/packages/tree/master/libs) entry) for InsightDock Charts'
size-trimmed core build. It is not built by this repository's own CI — it is a starting point for
your own feed.

## Use it

```bash
# in your OpenWrt buildroot / feed checkout
mkdir -p package/libs/insightdock
cp /path/to/insightdock/packaging/openwrt/Makefile package/libs/insightdock/Makefile
```

Then edit the copy:

1. Set `PKG_SOURCE_URL` to a real tagged release and replace `PKG_HASH:=skip` with the tarball's
   real `sha256sum`.
2. Adjust `INSIGHTDOCK_CHART_TYPES` / `INSIGHTDOCK_EXPORT_FORMATS` / `INSIGHTDOCK_FONT_MODE` to
   taste — these map directly onto the upstream Makefile's own feature-trim variables (see
   [`CLAUDE.md`](../../CLAUDE.md), "Make (core only — OpenWrt / minimal)").
3. Run `make menuconfig` in your buildroot, enable `Libraries > insightdock`, and build.

## Why the plain Makefile, not CMake

InsightDock Charts ships two build paths (see the root [README](../../README.md)): a CMake build for
desktop/server use, and a dependency-free plain `Makefile` with `CHART_TYPES` / `EXPORT_FORMATS` /
`FONT_MODE` knobs for trimming the compiled library down for size-constrained targets. The OpenWrt
package builds via the plain `Makefile`, since that's the path with the actual size-trimming
options routers care about.
