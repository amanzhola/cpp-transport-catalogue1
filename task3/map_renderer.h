// map_renderer.h
#pragma once

/**************************************************************************************************
 * ADDED:
 *   - namespace transport_catalogue::render
 *   - Глобальные using RenderBusSvg/RenderStopSvg для совместимости
 **************************************************************************************************/

#include <string>
#include <vector>

#include "transport_catalogue.h"

namespace transport_catalogue::render {

std::string RenderBusSvg(const transport_catalogue::domain::Bus& bus,
                         double width = 800.0,
                         double height = 600.0,
                         double padding = 50.0);

std::string RenderStopSvg(const transport_catalogue::domain::Stop& stop,
                          const std::vector<const transport_catalogue::domain::Bus*>& buses,
                          double width = 800.0,
                          double height = 600.0,
                          double padding = 50.0);

} // namespace transport_catalogue::render

// COMPAT
using transport_catalogue::render::RenderBusSvg;
using transport_catalogue::render::RenderStopSvg;
