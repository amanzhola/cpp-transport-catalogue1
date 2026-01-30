// stat_reader.cpp
#include "stat_reader.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

/**************************************************************************************************
 * ADDED:
 *   - namespace transport_catalogue::stat
 *   - detail::PrintBus / detail::PrintStop
 *
 * ❌ REMOVED:
 *   - глобальные static функции — теперь в detail (модульно и аккуратно)
 **************************************************************************************************/

namespace transport_catalogue::stat {

namespace detail {

static void PrintBus(const transport_catalogue::catalogue::TransportCatalogue& db,
                     std::string_view name, std::ostream& out) {
    const auto stat = db.GetBusStat(name);

    out << "Bus " << name << ": ";
    if (!stat.found) {
        out << "not found\n";
        return;
    }

    out << stat.stops_count << " stops on route, "
        << stat.unique_stops << " unique stops, "
        << std::setprecision(6) << stat.route_length << " route length\n";
}

static void PrintStop(const transport_catalogue::catalogue::TransportCatalogue& db,
                      std::string_view name, std::ostream& out) {
    const auto* stop = db.FindStop(name);
    if (!stop) {
        out << "Stop " << name << ": not found\n";
        return;
    }

    const auto& buses_set = db.GetBusesByStop(stop);

    if (buses_set.empty()) {
        out << "Stop " << name << ": no buses\n";
        return;
    }

    std::vector<std::string_view> bus_names;
    bus_names.reserve(buses_set.size());
    for (const auto* b : buses_set) {
        bus_names.push_back(b->name);
    }
    std::sort(bus_names.begin(), bus_names.end());

    out << "Stop " << name << ": buses";
    for (std::string_view bn : bus_names) {
        out << ' ' << bn;
    }
    out << '\n';
}

} // namespace detail


void ParseAndPrintStat(const transport_catalogue::catalogue::TransportCatalogue& db,
                       std::string_view req, std::ostream& out) {
    const auto sp = req.find(' ');
    if (sp == req.npos) {
        return;
    }

    const std::string_view kind(req.data(), sp);
    const std::string_view name = req.substr(sp + 1);

    if (kind == "Bus") {
        detail::PrintBus(db, name, out);
    } else if (kind == "Stop") {
        detail::PrintStop(db, name, out);
    }
}

} // namespace transport_catalogue::stat
