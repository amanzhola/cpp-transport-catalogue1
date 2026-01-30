// domain.h
#pragma once

/**************************************************************************************************
 * ADDED:
 *   - namespace transport_catalogue::domain
 *   - Stop/Bus/BusStat живут внутри domain
 *   - В конце файла глобальные алиасы (Stop/Bus/BusStat), чтобы тесты Task2 не упали
 **************************************************************************************************/

#include <cstddef>
#include <string>
#include <vector>

#include "geo.h"

namespace transport_catalogue::domain {

struct Stop {
    std::string name;
    transport_catalogue::geo::Coordinates coord{0.0, 0.0};
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip = false;
};

struct BusStat {
    std::size_t stops_count = 0;
    std::size_t unique_stops = 0;
    double route_length = 0.0;
    bool found = false;
};

} // namespace transport_catalogue::domain

// COMPAT
using Stop = transport_catalogue::domain::Stop;
using Bus = transport_catalogue::domain::Bus;
using BusStat = transport_catalogue::domain::BusStat;
