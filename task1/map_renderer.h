#pragma once
#pragma message("### USING map_renderer.h from: " __FILE__)

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace tc::render {

struct RenderSettings {
    double width = 1600.0;
    double height = 1000.0;
    double padding = 100.0;

    double stop_radius = 4.0;
    double line_width = 6.0;

    int label_font_size = 14;
    double line_opacity = 0.7;

    bool draw_stop_labels = true;
    bool draw_bus_end_label = true;

    std::string_view underlayer_color = "white";
    double underlayer_width = 4.0;
};

void RenderMapSvg(const tc::catalogue::TransportCatalogue& db,
                  std::ostream& out,
                  const RenderSettings& settings = {});

}  // namespace tc::render
