// transport_catalogue.h
#pragma once

#include <deque>
#include <string_view>
#include <unordered_map>

#include <vector>

#include "domain.h"   // <-- вместо geo.h и вместо структур

// Хешер для string_view (прозрачный), чтобы искать и по string, и по string_view
struct StrViewHasher {
    using is_transparent = void;
    std::size_t operator()(std::string_view s) const noexcept {
        return std::hash<std::string_view>{}(s);
    }
};


// ----- Каталог -----
class TransportCatalogue {
public:
    void AddStop(std::string name, Coordinates coord);
    void AddBus(std::string name, const std::vector<std::string_view>& stop_names);

    const Stop* FindStop(std::string_view name) const;
    const Bus*  FindBus (std::string_view name) const;

    BusStat GetBusStat(std::string_view bus_name) const;

    // вернуть список маршрутов в порядке добавления (1..N)
    const std::vector<const Bus*>& GetAllBuses() const;

    // получить маршрут по порядковому номеру (1..N), иначе nullptr
    const Bus* GetBusByIndex(std::size_t index) const;

private:
    // физическое хранение (стабильные адреса)
    std::deque<Stop> stops_;
    std::deque<Bus>  buses_;

    // индексы по имени
    std::unordered_map<std::string_view, const Stop*, StrViewHasher, std::equal_to<>> stop_by_name_;
    std::unordered_map<std::string_view, const Bus*,  StrViewHasher, std::equal_to<>> bus_by_name_;
   
    // список в SVG
    std::vector<const Bus*> bus_order_;
};

/*
#pragma once

class TransportCatalogue {
	// Реализуйте класс самостоятельно
    };*/

