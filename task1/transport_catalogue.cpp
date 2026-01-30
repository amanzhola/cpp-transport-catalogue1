#include "transport_catalogue.h"

#include <unordered_set>

#include "geo.h"

namespace tc::catalogue {

void TransportCatalogue::AddStop(std::string name, tc::geo::Coordinates coord) {
    stops_.push_back(Stop{std::move(name), coord});
    const Stop* ptr = &stops_.back();
    stop_by_name_[ptr->name] = ptr;
}

void TransportCatalogue::AddBus(std::string name,
                               const std::vector<std::string_view>& stop_names,
                               bool is_roundtrip) {
    buses_.push_back(Bus{});
    Bus& bus = buses_.back();
    bus.name = std::move(name);
    bus.is_roundtrip = is_roundtrip;

    bus.stops.reserve(stop_names.size());
    for (std::string_view stop_name : stop_names) {
        const Stop* stop_ptr = FindStop(stop_name);
        if (stop_ptr) {
            bus.stops.push_back(stop_ptr);
        }
    }

    bus_by_name_[bus.name] = &bus;
}

const Stop* TransportCatalogue::FindStop(std::string_view name) const {
    if (auto it = stop_by_name_.find(name); it != stop_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

const Bus* TransportCatalogue::FindBus(std::string_view name) const {
    if (auto it = bus_by_name_.find(name); it != bus_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

BusStat TransportCatalogue::GetBusStat(std::string_view bus_name) const {
    BusStat res;
    const Bus* bus = FindBus(bus_name);
    if (!bus) {
        return res;
    }

    res.found = true;

    // U
    std::unordered_set<const Stop*> uniq;
    uniq.reserve(bus->stops.size());
    for (const Stop* s : bus->stops) uniq.insert(s);
    res.unique_stops = uniq.size();

    const size_t n = bus->stops.size();
    if (n == 0) {
        return res;
    }

    if (bus->is_roundtrip) {
        // Кольцо: если во входе "A > ... > A", то R = n
        res.stops_count = n;

        double length = 0.0;
        for (size_t i = 1; i < n; ++i) {
            length += tc::geo::ComputeDistance(bus->stops[i - 1]->coord,
                                               bus->stops[i]->coord);
        }
        res.route_length = length;
    } else {
        // Некольцевой: хранится A-B-C (n), но реальный путь A..C..A => R = 2*n - 1
        res.stops_count = 2 * n - 1;

        double length = 0.0;
        // туда
        for (size_t i = 1; i < n; ++i) {
            length += tc::geo::ComputeDistance(bus->stops[i - 1]->coord,
                                               bus->stops[i]->coord);
        }
        // обратно (без underflow)
        for (size_t i = n; i-- > 1; ) { // i: n-1 ... 1
            length += tc::geo::ComputeDistance(bus->stops[i]->coord,
                                               bus->stops[i - 1]->coord);
        }
        res.route_length = length;
    }

    return res;
}

}  // namespace tc::catalogue
