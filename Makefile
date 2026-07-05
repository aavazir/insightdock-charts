# Portable Makefile — host build or OpenWrt cross-compile.
# Cross-compile example:
#   make CXX=mips-openwrt-linux-g++ AR=mips-openwrt-linux-ar \
#        CHART_TYPES="pie" EXPORT_FORMATS="png" FONT_MODE=system

CXX ?= g++
AR  ?= ar

# ── Feature selection ──────────────────────────────────────────────────────────
# Chart types to compile in (space-separated subset of: pie bar line scatter).
# Omit types you don't need to shrink the library.
CHART_TYPES    ?= pie bar line

# Export formats to compile in (space-separated subset of: png jpeg).
# png is always required. Omit jpeg to drop ~15 KB of JPEG codec.
EXPORT_FORMATS ?= png jpeg

# Font mode:
#   embedded — bakes a ~20 KB ASCII TTF into the library (zero runtime deps)
#   system   — no embedded font; call Font::load_from_file("/path/to/font.ttf")
#              at startup. Use when OpenWrt already ships a TTF in /www/assets/.
FONT_MODE      ?= embedded
# ──────────────────────────────────────────────────────────────────────────────

CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
INCLUDES  = -I include -I third_party

# ── Core sources (always compiled) ────────────────────────────────────────────
SRCS = \
  src/image_buffer.cc \
  src/palette.cc \
  src/font.cc \
  src/scale.cc \
  src/decimate.cc \
  src/export.cc \
  src/png_writer.cc \
  src/svg_writer.cc \
  src/tick_format.cc \
  src/spline.cc \
  src/marker.cc \
  src/bar_shape.cc \
  src/reference_lines.cc

# ── Chart types ───────────────────────────────────────────────────────────────
ifneq ($(filter pie,$(CHART_TYPES)),)
  SRCS     += src/pie_chart.cc src/svg_pie.cc
  CXXFLAGS += -DINSIGHTDOCK_HAS_PIE
endif

ifneq ($(filter bar line,$(CHART_TYPES)),)
  SRCS += src/cartesian_chart.cc
endif
ifneq ($(filter bar,$(CHART_TYPES)),)
  SRCS     += src/bar_chart.cc
  CXXFLAGS += -DINSIGHTDOCK_HAS_BAR
endif
ifneq ($(filter line,$(CHART_TYPES)),)
  SRCS     += src/line_chart.cc
  CXXFLAGS += -DINSIGHTDOCK_HAS_LINE
endif
ifneq ($(filter bar line,$(CHART_TYPES)),)
  SRCS += src/combo_chart.cc
endif
ifneq ($(filter scatter,$(CHART_TYPES)),)
  SRCS     += src/scatter_chart.cc
  CXXFLAGS += -DINSIGHTDOCK_HAS_SCATTER
endif

# ── Export formats ────────────────────────────────────────────────────────────
ifeq ($(filter jpeg,$(EXPORT_FORMATS)),)
  CXXFLAGS += -DINSIGHTDOCK_NO_JPEG
endif

# ── Font mode ─────────────────────────────────────────────────────────────────
ifeq ($(FONT_MODE),system)
  CXXFLAGS += -DINSIGHTDOCK_NO_EMBEDDED_FONT
endif

# ── Build ─────────────────────────────────────────────────────────────────────
OBJS = $(SRCS:.cc=.o)
LIB  = libinsightdock.a

.PHONY: all clean lib examples test

all: lib examples

lib: $(LIB)

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ── Examples (only built when required chart type is present) ─────────────────
EXAMPLES :=

ifneq ($(filter pie,$(CHART_TYPES)),)
  EXAMPLES += example_main
endif
ifneq ($(filter bar line,$(CHART_TYPES)),)
  EXAMPLES += bar_line_example
endif

examples: $(EXAMPLES)

example_main: examples/main.cc $(LIB)
	$(CXX) $(CXXFLAGS) $(INCLUDES) examples/main.cc $(LIB) -o $@

bar_line_example: examples/bar_line_example.cc $(LIB)
	$(CXX) $(CXXFLAGS) $(INCLUDES) examples/bar_line_example.cc $(LIB) -o $@

test: examples
	./example_main
	@test -f chart_output.png

clean:
	rm -f $(OBJS) $(LIB) example_main bar_line_example *.png *.jpg

# PoDoFo support requires CMake — see CLAUDE.md.
