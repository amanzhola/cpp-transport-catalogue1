#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "geo.h"

namespace tc::catalogue {

struct StrViewHasher {
    using is_transparent = void;
    std::size_t operator()(std::string_view s) const noexcept {
        return std::hash<std::string_view>{}(s);
    }
};

struct Stop {
    std::string name;
    tc::geo::Coordinates coord{};
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;  // КАК ВО ВХОДЕ (без "дорисовки обратно")
    bool is_roundtrip = false;       // true для '>', false для '-'
};

struct BusStat {
    std::size_t stops_count = 0;   // R
    std::size_t unique_stops = 0;  // U
    double route_length = 0.0;     // L
    bool found = false;
};

class TransportCatalogue {
public:
    void AddStop(std::string name, tc::geo::Coordinates coord);

    void AddBus(std::string name,
                const std::vector<std::string_view>& stop_names,
                bool is_roundtrip);

    const Stop* FindStop(std::string_view name) const;
    const Bus* FindBus(std::string_view name) const;

    BusStat GetBusStat(std::string_view bus_name) const;

    const std::deque<Stop>& GetStops() const { return stops_; }
    const std::deque<Bus>& GetBuses() const { return buses_; }

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;

    std::unordered_map<std::string_view, const Stop*, StrViewHasher, std::equal_to<>> stop_by_name_;
    std::unordered_map<std::string_view, const Bus*,  StrViewHasher, std::equal_to<>> bus_by_name_;
};

}  // namespace tc::catalogue
