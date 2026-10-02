// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#include "insightdock/palette.h"

namespace insightdock::charts {

namespace {

Color const kPalette[] = {
    Color::rgb(55, 99, 227),  Color::rgb(255, 99, 109),  Color::rgb(78, 203, 190),
    Color::rgb(255, 159, 67), Color::rgb(153, 102, 255), Color::rgb(255, 205, 86),
    Color::rgb(54, 162, 235), Color::rgb(201, 203, 207), Color::rgb(75, 192, 192),
    Color::rgb(255, 99, 132), Color::rgb(199, 199, 199), Color::rgb(81, 204, 164),
};

Color const kTableau10[] = {
    Color::rgb(78, 121, 167),  // #4E79A7
    Color::rgb(242, 142, 43),  // #F28E2B
    Color::rgb(225, 87, 89),   // #E15759
    Color::rgb(118, 183, 178), // #76B7B2
    Color::rgb(89, 161, 79),   // #59A14F
    Color::rgb(237, 201, 72),  // #EDC948
    Color::rgb(176, 122, 161), // #B07AA1
    Color::rgb(255, 157, 167), // #FF9DA7
    Color::rgb(156, 117, 95),  // #9C755F
    Color::rgb(186, 176, 172), // #BAB0AC
};

// Okabe & Ito (2008) colorblind-safe palette; black is left out so series stay distinct from text.
Color const kColorblind[] = {
    Color::rgb(0, 114, 178),   // blue
    Color::rgb(230, 159, 0),   // orange
    Color::rgb(0, 158, 115),   // bluish green
    Color::rgb(213, 94, 0),    // vermillion
    Color::rgb(86, 180, 233),  // sky blue
    Color::rgb(204, 121, 167), // reddish purple
    Color::rgb(240, 228, 66),  // yellow
    Color::rgb(117, 117, 117), // gray
};

} // namespace

Color palette_color(std::size_t index) {
  return kPalette[index % palette_size()];
}

Color tableau10_color(std::size_t index) {
  return kTableau10[index % tableau10_size()];
}

Color colorblind_color(std::size_t index) {
  return kColorblind[index % colorblind_size()];
}

Color palette_color_by_kind(PaletteKind kind, std::size_t index) {
  switch (kind) {
  case PaletteKind::Tableau10:
    return tableau10_color(index);
  case PaletteKind::ColorblindSafe:
    return colorblind_color(index);
  case PaletteKind::ChartJs:
    break;
  }
  return palette_color(index);
}

} // namespace insightdock::charts
