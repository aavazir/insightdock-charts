// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors

#ifndef INSIGHTDOCK_CHARTS_DETAIL_SVG_TARGET_H
#define INSIGHTDOCK_CHARTS_DETAIL_SVG_TARGET_H

#include "insightdock/chart.h"
#include "insightdock/draw_target.h"
#include "insightdock/export_macros.h"

#include <string>

namespace insightdock::charts::detail {

/** @brief @ref DrawTarget that records native SVG elements (`<rect>`, `<path>`, `<text>`…). */
class INSIGHTDOCK_API SvgTarget final : public DrawTarget {
public:
  SvgTarget(float width, float height);

  float width() const override {
    return w_;
  }
  float height() const override {
    return h_;
  }
  void clear(Color const &color) override;
  void fill_rect(float x, float y, float w, float h, Color const &color) override;
  void fill_rounded_rect(float x, float y, float w, float h, float radius,
                         Color const &color) override;
  void fill_polygon(Point const *points, std::size_t count, Color const &color) override;
  void fill_circle(float cx, float cy, float radius, Color const &color) override;
  void draw_line(float x0, float y0, float x1, float y1, float width, Color const &color) override;
  void draw_polyline(Point const *points, std::size_t count, float width,
                     Color const &color) override;
  void draw_polygon_outline(Point const *points, std::size_t count, float width,
                            Color const &color) override;
  void draw_dashed_line(float x0, float y0, float x1, float y1, float width, Color const &color,
                        std::vector<float> const &dash) override;
  void draw_dashed_polyline(Point const *points, std::size_t count, float width, Color const &color,
                            std::vector<float> const &dash) override;
  void draw_text(Font const &font, float x, float y, std::string const &text, Color const &color,
                 TextAlign align, TextBaseline baseline) override;
  void draw_text_rotated_ccw(Font const &font, float cx, float cy, std::string const &text,
                             Color const &color) override;
  void draw_text_rotated_cw(Font const &font, float cx, float cy, std::string const &text,
                            Color const &color) override;

  /** @brief The complete SVG document recorded so far. */
  std::string str() const;

private:
  void text_element(Font const &font, std::string const &text, Color const &color,
                    char const *anchor, float x, float y, char const *transform);

  float w_, h_;
  std::string body_;
};

/**
 * @brief Render @p chart as a native-vector SVG document.
 * @return The XML, or an empty string if the chart type has no @ref Chart::draw path (pie and
 *         scatter use their own pipelines).
 */
INSIGHTDOCK_API std::string build_chart_svg(Chart const &chart);

} // namespace insightdock::charts::detail

#endif // INSIGHTDOCK_CHARTS_DETAIL_SVG_TARGET_H
