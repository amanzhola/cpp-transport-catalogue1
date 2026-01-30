// transport_catalogue.cpp
#include "transport_catalogue.h"

#include <cassert>
#include <string>
#include <unordered_set>

/**************************************************************************************************
 * ADDED:
 *   - namespace transport_catalogue::catalogue
 *   - Явные ссылки на geo/domain
 *
 * ❌ REMOVED:
 *   - Никаких дублей bus_by_name_ и т.п.
 **************************************************************************************************/

namespace transport_catalogue::catalogue {

void TransportCatalogue::AddStop(std::string name, geo::Coordinates coord) {
    stops_.push_back(domain::Stop{std::move(name), coord});
    const domain::Stop* p = &stops_.back();
    stop_by_name_[p->name] = p;
    stop_order_.push_back(p);
}

const std::vector<const domain::Stop*>& TransportCatalogue::GetAllStops() const {
    return stop_order_;
}

const domain::Stop* TransportCatalogue::GetStopByIndex(std::size_t index) const {
    if (index == 0 || index > stop_order_.size()) {
        return nullptr;
    }
    return stop_order_[index - 1];
}

void TransportCatalogue::AddBus(std::string name, const std::vector<std::string_view>& stop_names) {
    buses_.push_back(domain::Bus{});
    domain::Bus& b = buses_.back();
    b.name = std::move(name);

    b.stops.reserve(stop_names.size());

    for (std::string_view sv : stop_names) {
        const domain::Stop* s = FindStop(sv);
        assert(s != nullptr && "Stop not found while adding bus (input should be valid)");

        if (s) {
            b.stops.push_back(s);
            buses_by_stop_[s].insert(&b);
        }
    }

    bus_by_name_[b.name] = &b;
    bus_order_.push_back(&b);
}

const std::unordered_set<const domain::Bus*>& TransportCatalogue::GetBusesByStop(const domain::Stop* stop) const {
    static const std::unordered_set<const domain::Bus*> kEmpty;

    if (!stop) {
        return kEmpty;
    }

    auto it = buses_by_stop_.find(stop);
    return (it == buses_by_stop_.end()) ? kEmpty : it->second;
}

const std::vector<const domain::Bus*>& TransportCatalogue::GetAllBuses() const {
    return bus_order_;
}

const domain::Bus* TransportCatalogue::GetBusByIndex(std::size_t index) const {
    if (index == 0 || index > bus_order_.size()) {
        return nullptr;
    }
    return bus_order_[index - 1];
}

const domain::Stop* TransportCatalogue::FindStop(std::string_view name) const {
    if (auto it = stop_by_name_.find(name); it != stop_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

const domain::Bus* TransportCatalogue::FindBus(std::string_view name) const {
    if (auto it = bus_by_name_.find(name); it != bus_by_name_.end()) {
        return it->second;
    }
    return nullptr;
}

domain::BusStat TransportCatalogue::GetBusStat(std::string_view bus_name) const {
    domain::BusStat res;
    const domain::Bus* bus = FindBus(bus_name);
    if (!bus) {
        return res;
    }

    res.found = true;
    res.stops_count = bus->stops.size();

    std::unordered_set<const domain::Stop*> uniq;
    uniq.reserve(bus->stops.size());
    for (const domain::Stop* s : bus->stops) {
        uniq.insert(s);
    }
    res.unique_stops = uniq.size();

    double length = 0.0;
    for (std::size_t i = 1; i < bus->stops.size(); ++i) {
        length += transport_catalogue::geo::ComputeDistance(bus->stops[i - 1]->coord, bus->stops[i]->coord);
    }
    res.route_length = length;

    return res;
}

} // namespace transport_catalogue::catalogue
