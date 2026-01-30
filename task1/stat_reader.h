#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace tc::io {

void ParseAndPrintStat(const tc::catalogue::TransportCatalogue& catalogue,
                       std::string_view request,
                       std::ostream& output);

}  // namespace tc::io
