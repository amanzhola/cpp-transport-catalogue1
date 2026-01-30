// stat_reader.h
#pragma once

/**************************************************************************************************
 * ADDED:
 *   - namespace transport_catalogue::stat
 *   - Глобальный using ParseAndPrintStat для совместимости
 **************************************************************************************************/

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace transport_catalogue::stat {

void ParseAndPrintStat(const transport_catalogue::catalogue::TransportCatalogue& transport_catalogue,
                       std::string_view request,
                       std::ostream& output);

} // namespace transport_catalogue::stat

// COMPAT
using transport_catalogue::stat::ParseAndPrintStat;
