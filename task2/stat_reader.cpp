// stat_reader.cpp
#include "stat_reader.h"

#include <algorithm>   // sort
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

// Печать статистики по автобусу (Bus X)
static void PrintBus(const TransportCatalogue& db, std::string_view name, std::ostream& out) {
    const auto stat = db.GetBusStat(name);

    out << "Bus " << name << ": ";
    if (!stat.found) {
        out << "not found\n";
        return;
    }

    out << stat.stops_count << " stops on route, "
        << stat.unique_stops << " unique stops, "
        << std::setprecision(6) << stat.route_length << " route length\n";
}

// Печать информации по остановке (Stop X) — Task2
static void PrintStop(const TransportCatalogue& db, std::string_view name, std::ostream& out) {
    // 1) Проверяем, есть ли остановка вообще
    const Stop* stop = db.FindStop(name);
    if (!stop) {
        out << "Stop " << name << ": not found\n";
        return;
    }

    // 2) Берём набор автобусов по Stop*
    const auto& buses_set = db.GetBusesByStop(stop);

    // 3) Если набор пустой — остановка есть, но автобусов нет
    if (buses_set.empty()) {
        out << "Stop " << name << ": no buses\n";
        return;
    }

    // 4) Иначе собираем имена автобусов, сортируем по алфавиту
    std::vector<std::string_view> bus_names;
    bus_names.reserve(buses_set.size());
    for (const Bus* b : buses_set) {
        bus_names.push_back(b->name);
    }
    std::sort(bus_names.begin(), bus_names.end());

    // 5) Печатаем в формате: "Stop X: buses 256 828"
    out << "Stop " << name << ": buses";
    for (std::string_view bn : bus_names) {
        out << ' ' << bn;
    }
    out << '\n';
}

void ParseAndPrintStat(const TransportCatalogue& db, std::string_view req, std::ostream& out) {
    // Ожидаем формат: "<Kind> <Name>", где Kind = "Bus" или "Stop"
    const auto sp = req.find(' ');
    if (sp == req.npos) {
        return;
    }

    const std::string_view kind = std::string_view(req.data(), sp);
    const std::string_view name = req.substr(sp + 1);

    if (kind == "Bus") {
        PrintBus(db, name, out);
    } else if (kind == "Stop") {
        PrintStop(db, name, out);
    }
    // если что-то другое — молча игнорируем
}
