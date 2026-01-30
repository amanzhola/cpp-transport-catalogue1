// domain.h
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "geo.h"

// ----- Модели данных -----
struct Stop {
    std::string name;
    Coordinates coord{0.0, 0.0};
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops; // последовательность остановок полного пути
    bool is_roundtrip = false;      // на будущее
};

struct BusStat {
    std::size_t stops_count = 0;   // R
    std::size_t unique_stops = 0;  // U
    double route_length = 0.0;     // L
    bool found = false;
};
