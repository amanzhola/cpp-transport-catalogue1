#include "stat_reader.h"

#include <iomanip>
#include <iostream>
#include <string_view>

namespace {

#ifdef TC_VERBOSE
#define TC_LOG(x) do { std::cerr << x << '\n'; } while(false)
#else
#define TC_LOG(x) do {} while(false)
#endif

}  // namespace

namespace tc::io {

void ParseAndPrintStat(const tc::catalogue::TransportCatalogue& catalogue,
                       std::string_view request,
                       std::ostream& output) {
    // Поддерживаем только "Bus X"
    const std::size_t sp = request.find(' ');
    if (sp == request.npos || request.substr(0, sp) != "Bus") {
        return;
    }

    const std::string_view bus_name = request.substr(sp + 1);
    TC_LOG("🔎 Stat request: Bus '" << bus_name << "'");

    const auto stat = catalogue.GetBusStat(bus_name);

    output << "Bus " << bus_name << ": ";
    if (!stat.found) {
        output << "not found\n";
        TC_LOG("❌ Not found");
        return;
    }

    output << stat.stops_count << " stops on route, "
           << stat.unique_stops << " unique stops, "
           << std::setprecision(6) << stat.route_length << " route length\n";

    TC_LOG("✅ Found: R=" << stat.stops_count
          << " U=" << stat.unique_stops
          << " L=" << std::setprecision(6) << stat.route_length);
}

}  // namespace tc::io
