# Thread safety and ABI policy

## Thread safety

- **Reading a chart from many threads is safe.** `render()` is `const`, and
  `ChartExporter::render()/save()/encode()` never modify the chart you pass in — supersampled
  exports render from an internal copy (`Chart::clone()`) through a scaled drawing target, so exporting one chart from
  several threads at once is fine as long as nobody mutates it concurrently.
- **Mutating a chart while another thread reads it is not safe**, like any standard container.
- Distinct charts are fully independent; there is no global state.
- `save()` and `encode()` accept const charts directly. Mutable overloads delegate to the same
  implementation. Custom renderers and formatter callbacks must themselves permit concurrent
  reads; const methods can still change mutable or external state.
- Custom `Chart` subclasses should override `clone()` (`return std::make_unique<MyChart>(*this);`).
  Without it, supersampled rendering returns an empty buffer and `save()`/`encode()` report
  `RenderFailure`; exporting never writes through a const chart.

## Supersampling

Use `ExportOptions::supersample = 2` or `4` for smoother raster edges. The output stays at the
configured width and height. Factors are clamped to [1, 8] and the image pixel cap applies before
allocation at the higher resolution. Built-in charts preserve layout, wrapping, legend overflow,
pie callout selection, mark sizes, strokes, dashes, corner radii and all text styles.
Native SVG/PDF output ignores supersampling.
When a custom chart declines native drawing, allowed raster fallback uses the requested
supersampling and checks its larger pixel allocation before rendering.

Custom charts should implement both `clone()` and `draw(DrawTarget&)` for this scaling behavior.
For a `render()`-only custom chart, the compatibility path scales common canvas/font/margin
options; the custom renderer remains responsible for scaling its other pixel-space styles.

## Exception contracts

`save()` and `encode()` translate standard exceptions during raster rendering or cloning into
`RenderFailure`, and during native drawing or encoding into `EncodeFailure`. File I/O exceptions
become `BadPath`. They are not `noexcept`: non-standard exceptions from custom code propagate,
and allocations/path preparation outside guarded operations may throw. `render()` and the void
PDF embedding APIs propagate exceptions. Strict native export returns `VectorUnsupported` from
`save()`/`encode()`; strict PDF embedding throws `std::runtime_error`.

## Versioning and ABI

- The library follows semantic versioning for **source compatibility**: code that compiles against
  1.x keeps compiling against later 1.y.
- **Binary (ABI) compatibility is not promised.** Public option structs gain fields between minor
  releases; rebuild your code when upgrading between minor releases. The shared library's
  `SOVERSION` is always `major.minor`, including after 1.0, so an incompatible minor does not
  replace a library with the same identity. Patch releases must preserve the ABI within a minor.
- `find_package(InsightDock)` uses `SameMinorVersion` matching before 1.0 and `SameMajorVersion`
  from 1.0.
- Shared builds hide all symbols by default; every public class and function is marked
  `INSIGHTDOCK_API` (`insightdock/export_macros.h`), so Windows DLLs export the full API.
  Static builds define `INSIGHTDOCK_STATIC` automatically through the CMake target.
- Headers under `insightdock/detail/` are installed because some are used by tests, but they are
  **not** part of the stable API.
