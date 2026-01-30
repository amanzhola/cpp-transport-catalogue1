// transport_catalogue.h
#pragma once

/**************************************************************************************************
 * TASK 3: Неймспейсы
 *
 * ADDED:
 *   - Весь код каталога перенесён в namespace transport_catalogue::catalogue
 *   - Типы из domain/geo используются через префиксы (domain::Stop, geo::Coordinates и т.д.)
 *   - В конце файла оставлены глобальные алиасы для совместимости с тестами Task2:
 *       using TransportCatalogue = transport_catalogue::catalogue::TransportCatalogue;
 *
 * ❌ REMOVED / CHANGED:
 *   - Убрано "всё в глобальном пространстве имён" (раньше было глобально)
 *   - Теперь наружу “публично” торчит только TransportCatalogue (через алиас),
 *     а реализация/структура прячется за namespaces.
 **************************************************************************************************/

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "domain.h"  // domain::Stop, domain::Bus, domain::BusStat
#include "geo.h"     // geo::Coordinates, geo::ComputeDistance

namespace transport_catalogue::catalogue {

// Хешер для string_view (прозрачный), чтобы искать и по string, и по string_view
// ✅ ADDED: помещён в namespace transport_catalogue::catalogue
struct StrViewHasher {
    using is_transparent = void;
    std::size_t operator()(std::string_view s) const noexcept {
        return std::hash<std::string_view>{}(s);
    }
};

// ----- Каталог -----
class TransportCatalogue {
public:
    void AddStop(std::string name, geo::Coordinates coord);
    void AddBus (std::string name, const std::vector<std::string_view>& stop_names);

    const domain::Stop* FindStop(std::string_view name) const;
    const domain::Bus*  FindBus (std::string_view name) const;

    domain::BusStat GetBusStat(std::string_view bus_name) const;

    // ===================== Task2: Stop X =====================
    // Возвращает набор автобусов, проходящих через КОНКРЕТНУЮ остановку.
    // - Если остановка не встречалась ни в одном маршруте -> вернётся пустой set.
    // - Метод возвращает ссылку, поэтому для "пустого ответа" используется static-пустой set.
    const std::unordered_set<const domain::Bus*>& GetBusesByStop(const domain::Stop* stop) const;

    // ===================== SVG список (интерактив) =====================
    // вернуть список маршрутов в порядке добавления (1..N)
    const std::vector<const domain::Bus*>& GetAllBuses() const;

    // получить маршрут по порядковому номеру (1..N), иначе nullptr
    const domain::Bus* GetBusByIndex(std::size_t index) const;

    // вернуть список остановок в порядке добавления (1..N)
    const std::vector<const domain::Stop*>& GetAllStops() const;

    // получить остановку по порядковому номеру (1..N), иначе nullptr
    const domain::Stop* GetStopByIndex(std::size_t index) const;

private:
    // физическое хранение (стабильные адреса, deque не "переезжает" как vector)
    std::deque<domain::Stop> stops_;
    std::deque<domain::Bus>  buses_;

    // индексы по имени (быстрый поиск)
    std::unordered_map<std::string_view, const domain::Stop*, StrViewHasher, std::equal_to<>> stop_by_name_;
    std::unordered_map<std::string_view, const domain::Bus*,  StrViewHasher, std::equal_to<>> bus_by_name_;

    // ===================== Task2: индекс Stop* -> множество Bus* =====================
    // Ключ: указатель на Stop (адрес элемента в stops_)
    // Значение: набор указателей на Bus (адреса элементов в buses_)
    // unordered_set гарантирует отсутствие дублей.
    std::unordered_map<const domain::Stop*, std::unordered_set<const domain::Bus*>> buses_by_stop_;

    // список в SVG (порядок добавления автобусов)
    std::vector<const domain::Bus*> bus_order_;

    // список остановок для интерактива (порядок добавления)
    std::vector<const domain::Stop*> stop_order_;
};

}  // namespace transport_catalogue::catalogue


/**************************************************************************************************
 * COMPATIBILITY LAYER (чтобы автотесты Task2 не сломались)
 *
 * Автотесты прошлой задачи обычно используют глобальные имена:
 *   TransportCatalogue, Stop, Bus, BusStat, Coordinates, ComputeDistance, ...
 *
 * Поэтому:
 *   - сами реализации лежат в namespaces
 *   - но мы оставляем глобальные алиасы (это не дублирование кода!)
 **************************************************************************************************/
using TransportCatalogue = transport_catalogue::catalogue::TransportCatalogue;
// StrViewHasher тесты обычно не используют, но можно оставить при желании:
// using StrViewHasher = transport_catalogue::catalogue::StrViewHasher;
