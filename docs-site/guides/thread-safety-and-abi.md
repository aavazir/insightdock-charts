# Thread safety and ABI policy

## Thread safety

- **Reading a chart from many threads is safe.** `render()` is `const`, and
  `ChartExporter::render()/save()/encode()` never modify the chart you pass in — supersampled
  exports render from an internal scaled copy (`Chart::clone()`), so exporting one chart from
  several threads at once is fine as long as nobody mutates it concurrently.
- **Mutating a chart while another thread reads it is not safe**, like any standard container.
- Distinct charts are fully independent; there is no global state.
- Custom `Chart` subclasses should override `clone()` (`return std::make_unique<MyChart>(*this);`).
  Without it, supersampled export falls back to temporarily rewriting the chart's options, which
  is **not** thread-safe.

## Versioning and ABI

- The library follows semantic versioning for **source compatibility**: code that compiles against
  1.x keeps compiling against later 1.y.
- **Binary (ABI) compatibility is not promised.** Public option structs gain fields between minor
  releases; rebuild your code when upgrading. The shared library's `SOVERSION` is `major.minor`
  while the major version is 0, and the major alone from 1.0.
- `find_package(InsightDock)` uses `SameMinorVersion` matching before 1.0 and `SameMajorVersion`
  from 1.0.
- Shared builds hide all symbols by default; every public class and function is marked
  `INSIGHTDOCK_API` (`insightdock/export_macros.h`), so Windows DLLs export the full API.
  Static builds define `INSIGHTDOCK_STATIC` automatically through the CMake target.
- Headers under `insightdock/detail/` are installed because some are used by tests, but they are
  **not** part of the stable API.
