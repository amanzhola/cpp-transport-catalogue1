// transport_catalogue.h
#pragma once

#include <deque>
#include <string_view>
#include <unordered_map>
#include <unordered_set>   // <-- нужно для unordered_set<const Bus*>
#include <vector>

#include "domain.h"   // Stop, Bus, Coordinates, BusStat, ComputeDistance

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
    void AddBus (std::string name, const std::vector<std::string_view>& stop_names);

    const Stop* FindStop(std::string_view name) const;
    const Bus*  FindBus (std::string_view name) const;

    BusStat GetBusStat(std::string_view bus_name) const;

    // ===================== Task2: Stop X =====================
    // Возвращает набор автобусов, проходящих через КОНКРЕТНУЮ остановку.
    // - Если остановка не встречалась ни в одном маршруте -> вернётся пустой set.
    // - Метод возвращает ссылку, поэтому для "пустого ответа" используется static-пустой set.
    const std::unordered_set<const Bus*>& GetBusesByStop(const Stop* stop) const;

    // ===================== Ваш функционал (SVG список) =====================
    // вернуть список маршрутов в порядке добавления (1..N)
    const std::vector<const Bus*>& GetAllBuses() const;

    // получить маршрут по порядковому номеру (1..N), иначе nullptr
    const Bus* GetBusByIndex(std::size_t index) const;

    // вернуть список остановок в порядке добавления (1..N)
    const std::vector<const Stop*>& GetAllStops() const;

    // получить остановку по порядковому номеру (1..N), иначе nullptr
    const Stop* GetStopByIndex(std::size_t index) const;

private:
    // физическое хранение (стабильные адреса, deque не "переезжает" как vector)
    std::deque<Stop> stops_;
    std::deque<Bus>  buses_;

    // индексы по имени (быстрый поиск)
    std::unordered_map<std::string_view, const Stop*, StrViewHasher, std::equal_to<>> stop_by_name_;
    std::unordered_map<std::string_view, const Bus*,  StrViewHasher, std::equal_to<>> bus_by_name_;

    // ===================== Task2: индекс Stop* -> множество Bus* =====================
    // Ключ: указатель на Stop (адрес элемента в stops_)
    // Значение: набор указателей на Bus (адреса элементов в buses_)
    // unordered_set гарантирует отсутствие дублей.
    std::unordered_map<const Stop*, std::unordered_set<const Bus*>> buses_by_stop_;

    // список в SVG (порядок добавления автобусов)
    std::vector<const Bus*> bus_order_;

    // список остановок для интерактива (порядок добавления)
    std::vector<const Stop*> stop_order_;
};
