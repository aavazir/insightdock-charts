// SPDX-License-Identifier: MIT
// Copyright (c) InsightDock contributors
//
// Generates all_charts_raster.pdf — one page per chart type, raster-embedded
// at 2x supersample so zooming reveals pixel quality.

#include "insightdock/charts.h"
#include "insightdock/pdf/podofo.h"

#include <podofo/podofo.h>
#include <iostream>

using namespace PoDoFo;
using namespace insightdock::charts;

namespace {

// Finish the current page, create a new A4 page, embed the chart centred with
// a 36pt margin on all sides.  Call painter.FinishDrawing() once after the
// last chart to close the final page.
void add_page(PdfMemDocument &doc, PdfPainter &painter, Chart &chart,
              ExportOptions const &opts, bool first_page = false) {
  if (!first_page)
    painter.FinishDrawing();

  auto &page = doc.GetPages().CreatePage(PdfPageSize::A4);
  painter.SetCanvas(page);

  auto box   = page.GetMediaBox();
  double mar  = 36.0;
  double max_w = box.Width  - 2.0 * mar;
  double max_h = box.Height - 2.0 * mar;
  double asp  = static_cast<double>(chart.options().width) /
                static_cast<double>(chart.options().height);
  double w = max_w, h = w / asp;
  if (h > max_h) { h = max_h; w = h * asp; }
  double x = mar + (max_w - w) * 0.5;
  double y = mar + (max_h - h) * 0.5;
  ChartExporter::draw_pdf(doc, painter, chart, x, y, w, h, opts);
}

} // namespace

int main() {
  ExportOptions raster;
  raster.supersample = 2;

  PdfMemDocument doc;
  PdfPainter    painter;

  // ── Page 1: Bar Chart ──────────────────────────────────────────────────────
  {
    BarChart bar;
    bar.options().width  = 800;
    bar.options().height = 480;
    bar.options().title  = "Monthly Website Sessions by Device";
    bar.cartesian_options().y_axis_label = "Sessions";
    bar.cartesian_options().x_axis_label = "Month";
    bar.options().legend.enabled = true;
    bar.options().show_grid = true;
    bar.set_categories({"Jan","Feb","Mar","Apr","May","Jun",
                        "Jul","Aug","Sep","Oct","Nov","Dec"});
    bar.add_series("Desktop", {4200,3900,4500,4800,5100,5400,
                               5200,5600,5300,5700,6100,6800});
    bar.add_series("Mobile",  {3100,3400,3800,4100,4600,4900,
                               5300,5100,5500,5800,6200,7100});
    bar.add_series("Tablet",  { 900, 850,1000,1100,1200,1300,
                               1250,1400,1350,1450,1600,1900});
    add_page(doc, painter, bar, raster, /*first_page=*/true);
  }

  // ── Page 2: Line Chart ────────────────────────────────────────────────────
  {
    LineChart line;
    line.options().width  = 800;
    line.options().height = 480;
    line.options().title  = "WAN Interface Bandwidth (Mbps)";
    line.cartesian_options().y_axis_label = "Mbps";
    line.cartesian_options().x_axis_label = "Hour";
    line.options().legend.enabled = true;
    line.options().show_grid = true;
    line.set_categories({"00:00","02:00","04:00","06:00","08:00","10:00",
                         "12:00","14:00","16:00","18:00","20:00","22:00"});
    line.add_series("Download", {12, 8, 6,18,72,95,88,102,97,85,68,45});
    line.add_series("Upload",   { 4, 3, 2, 7,28,38,35, 41,39,32,26,18});
    add_page(doc, painter, line, raster);
  }

  // ── Page 3: Area Chart ────────────────────────────────────────────────────
  {
    AreaChart area;
    area.options().width  = 800;
    area.options().height = 480;
    area.options().title  = "Server Resource Utilization (%)";
    area.cartesian_options().y_axis_label = "Utilization %";
    area.cartesian_options().x_axis_label = "Time";
    area.options().legend.enabled = true;
    area.options().show_grid = true;
    area.set_categories({"00:00","03:00","06:00","09:00","12:00","15:00",
                         "18:00","21:00","24:00"});
    area.add_series("CPU",    {18,14,12,58,85,91,80,65,35});
    area.add_series("Memory", {52,50,48,55,60,62,60,55,50});
    add_page(doc, painter, area, raster);
  }

  // ── Page 4: Combo Chart ───────────────────────────────────────────────────
  {
    ComboChart combo;
    combo.options().width  = 800;
    combo.options().height = 480;
    combo.options().title  = "Daily Active Users & Avg. Session Duration";
    combo.cartesian_options().y_axis_label  = "Active Users";
    combo.cartesian_options().y2_axis_label = "Avg. Duration (min)";
    combo.cartesian_options().x_axis_label = "Day";
    combo.options().legend.enabled = true;
    combo.options().show_grid = true;
    combo.set_categories({"Mon","Tue","Wed","Thu","Fri","Sat","Sun"});
    combo.add_bar_series("Active Users", {320,415,390,445,510,280,195});
    // Different units and magnitude than Active Users — drawn against the
    // secondary (right) y-axis instead of sharing the bar series' scale.
    combo.add_line_series("Avg. Duration (min)", {4.2,5.1,3.8,6.0,5.5,7.2,6.8},
                          Color::rgb(192, 75, 75), /*secondary_axis=*/true);
    add_page(doc, painter, combo, raster);
  }

  // ── Page 5: Pie Chart ─────────────────────────────────────────────────────
  // Canvas: 1400x1600 (0.875 aspect ≈ portrait) so it fills ~77% of A4 height.
  // Explicit radius=400 leaves ~100px on each side for callout labels.
  {
    PieChart pie;
    pie.options().width      = 1400;
    pie.options().height     = 1600;
    pie.options().title      = "Traffic Breakdown by Protocol";
    pie.options().font_size  = 20.0f;
    pie.options().background = Color::white();

    PieOptions po;
    po.center_x        = 700.0;   // explicit — bypass set_pie_options auto-compute
    po.center_y        = 800.0;   // explicit
    po.radius          = 400.0;   // leaves ~100px margin on each side for callout labels
    po.label_font_size = 22.0f;
    po.show_labels     = true;
    po.show_percentage = true;
    po.show_legend     = false;
    pie.set_pie_options(po);

    pie.add_slice("HTTPS",  62.4);
    pie.add_slice("HTTP",   14.1);
    pie.add_slice("DNS",     9.7);
    pie.add_slice("QUIC",    7.2);
    pie.add_slice("Other",   6.6);
    add_page(doc, painter, pie, raster);
  }

  painter.FinishDrawing();
  doc.Save("all_charts_raster.pdf");
  std::cout << "Wrote all_charts_raster.pdf (5 pages, 2x supersample raster)\n";
  return 0;
}
