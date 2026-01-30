// transport_catalogue.cpp
#include "transport_catalogue.h"

#include <cassert>
#include <string>
#include <unordered_set>

void TransportCatalogue::AddStop(std::string name, Coordinates coord) {
    // 1) Кладём объект Stop в физическое хранилище
    stops_.push_back(Stop{std::move(name), coord});

    // 2) Берём стабильный адрес (deque гарантирует стабильность адресов)
    const Stop* p = &stops_.back();

    // 3) Индексируем по имени: "имя" -> Stop*
    stop_by_name_[p->name] = p;

    stop_order_.push_back(p);   // для интерактивного списка остановок
}

const std::vector<const Stop*>& TransportCatalogue::GetAllStops() const {
    return stop_order_;
}

const Stop* TransportCatalogue::GetStopByIndex(std::size_t index) const {
    if (index == 0 || index > stop_order_.size()) {
        return nullptr;
    }
    return stop_order_[index - 1];
}

void TransportCatalogue::AddBus(std::string name, const std::vector<std::string_view>& stop_names) {
    // 1) Создаём автобус в физическом хранилище
    buses_.push_back(Bus{});
    Bus& b = buses_.back();
    b.name = std::move(name);

    // 2) Заполняем маршрут указателями на Stop
    b.stops.reserve(stop_names.size());

    for (std::string_view sv : stop_names) {
        // Ищем Stop* по имени через индекс
        const Stop* s = FindStop(sv);

        // По условию учебного проекта обычно Stop уже добавлены перед Bus,
        // поэтому s не должен быть nullptr. Оставляем assert для отладки.
        assert(s != nullptr && "Stop not found while adding bus (input should be valid)");

        if (s) {
            b.stops.push_back(s);

            // ===================== Task2: заполняем индекс stop -> buses =====================
            // Теперь по остановке можно будет быстро получить список автобусов.
            // unordered_set не допускает дублей автоматически.
            buses_by_stop_[s].insert(&b);
        }
    }

    // 3) Индексируем автобус по имени: "256" -> Bus*
    bus_by_name_[b.name] = &b;

    // ===================== Ваш функционал (SVG список) =====================
    // ВАЖНО: второй раз bus_by_name_ не нужен (у вас он был продублирован).
    // Оставляем только добавление в bus_order_.
    bus_order_.push_back(&b);
}

// ===================== Task2: Stop X =====================
const std::unordered_set<const Bus*>& TransportCatalogue::GetBusesByStop(const Stop* stop) const {
    // Возвращаем ссылку. Если ключа нет — возвращаем ссылку на статический пустой set.
    static const std::unordered_set<const Bus*> kEmpty;

    if (!stop) {
        return kEmpty;
    }

    auto it = buses_by_stop_.find(stop);
    return (it == buses_by_stop_.end()) ? kEmpty : it->second;
}

// ===================== Ваш функционал (SVG список) =====================
const std::vector<const Bus*>& TransportCatalogue::GetAllBuses() const {
    return bus_order_;
}

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

    // длина маршрута (сумма расстояний между соседними остановками)
    double length = 0.0;
    for (std::size_t i = 1; i < bus->stops.size(); ++i) {
        length += ComputeDistance(bus->stops[i - 1]->coord, bus->stops[i]->coord);
    }
    res.route_length = length;

    return res;
}
