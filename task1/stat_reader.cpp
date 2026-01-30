// stat_reader.cpp
/*
#include "stat_reader.h"

void ParseAndPrintStat(const TransportCatalogue& tansport_catalogue, std::string_view request,
                       std::ostream& output) {
    // Реализуйте самостоятельно
}
*/
#include "stat_reader.h"

#include <iomanip>
#include <iostream>
#include <string_view>

void ParseAndPrintStat(const TransportCatalogue& db, std::string_view req, std::ostream& out) {
    // ожидаем формат: "Bus <имя_маршрута>"
    const auto sp = req.find(' ');
    if (sp == req.npos || std::string_view(req.data(), sp) != std::string_view("Bus")) {
        return; // в этой части поддерживаем только Bus
    }

    std::string_view bus_name = req.substr(sp + 1);

    const auto stat = db.GetBusStat(bus_name);

    out << "Bus " << bus_name << ": ";
    if (!stat.found) {
        out << "not found\n";
        return;
    }

    out << stat.stops_count << " stops on route, "
        << stat.unique_stops << " unique stops, "
        << std::setprecision(6) << stat.route_length << " route length\n";
}
