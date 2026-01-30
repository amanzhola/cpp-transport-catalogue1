// transport_catalogue.cpp
#include "transport_catalogue.h"

#include <string>
#include <unordered_set>
#include <cassert> 

void TransportCatalogue::AddStop(std::string name, Coordinates coord) {
    stops_.push_back(Stop{std::move(name), coord});
    const Stop* p = &stops_.back();
    stop_by_name_[p->name] = p;
}

void TransportCatalogue::AddBus(std::string name, const std::vector<std::string_view>& stop_names) {
    buses_.push_back(Bus{});
    Bus& b = buses_.back();
    b.name = std::move(name);
    b.stops.reserve(stop_names.size());
    for (std::string_view sv : stop_names) {
        if (const Stop* s = FindStop(sv)) {
	    assert(s != nullptr && "Stop not found while adding bus (input should be valid)");
            b.stops.push_back(s);
        }
    }
    bus_by_name_[b.name] = &b;
    
    // для списка SVG
    bus_by_name_[b.name] = &b;
    bus_order_.push_back(&b);
}

// для списка SVG
const std::vector<const Bus*>& TransportCatalogue::GetAllBuses() const {
    return bus_order_;
}

// для списка SVG
const Bus* TransportCatalogue::GetBusByIndex(std::size_t index) const {
    if (index == 0 || index > bus_order_.size()) {
        return nullptr;
    }
    return bus_order_[index - 1];
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
        return res; // found=false
    }

    res.found = true;
    res.stops_count = bus->stops.size();

    // уникальные остановки
    std::unordered_set<const Stop*> uniq;
    uniq.reserve(bus->stops.size());
    for (const Stop* s : bus->stops) {
        uniq.insert(s);
    }
    res.unique_stops = uniq.size();

    // длина маршрута (сумма соседних расстояний)
    double length = 0.0;
    for (std::size_t i = 1; i < bus->stops.size(); ++i) {
        length += ComputeDistance(bus->stops[i - 1]->coord, bus->stops[i]->coord);
    }
    res.route_length = length;

    return res;
}
