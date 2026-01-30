// main.cpp
#include <iostream>
#include <string>

#include <fstream>
#include <limits>
#include <cctype>
#include <iomanip> // для setprecision(6)

#include "input_reader.h"
#include "stat_reader.h"

#ifdef INTERACTIVE
#include "map_renderer.h"
#endif

using namespace std;

int main(int argc, char** argv) {
    TransportCatalogue catalogue;

    // ---------- Источник ввода базы ----------
    istream* input = &cin;
    ifstream fin;

#ifdef INTERACTIVE
    // В интерактивном режиме база может читаться из файла:
    // запуск: transport_catalogue.exe input.txt
    if (argc >= 2) {
        fin.open(argv[1]);
        if (!fin) {
            cout << "Cannot open file: " << argv[1] << "\n";
            return 1;
        }
        input = &fin;
    }
#endif

    // ---------- Чтение и применение запросов базы ----------
    int base_request_count = 0;
    (*input) >> base_request_count >> ws;

    {
        InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            string line;
            getline(*input, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

#ifndef INTERACTIVE
    // ---------- Обычный режим (как в задании): читаем stat-запросы из input ----------
    int stat_request_count = 0;
    (*input) >> stat_request_count >> ws;

    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(*input, line);
        ParseAndPrintStat(catalogue, line, cout);
    }
#else
    // -------- Интерактивный режим: показываем Bus-статистику и принимаем имя маршрута --------
    const auto& buses = catalogue.GetAllBuses();

    cout << "Found routes: " << buses.size() << "\n\n";
    if (buses.empty()) {
        cout << "No routes to show.\n";
        return 0;
    }

    // Печатаем список маршрутов в формате как в задании + порядковый номер
    for (size_t i = 0; i < buses.size(); ++i) {
        const auto* bus = buses[i];
        const auto stat = catalogue.GetBusStat(bus->name);

        cout << (i + 1) << ") "
             << "Bus " << bus->name << ": "
             << stat.stops_count << " stops on route, "
             << stat.unique_stops << " unique stops, "
             << setprecision(6) << stat.route_length << " route length\n";
    }

    auto MakeSafeFilename = [](string s) {
        for (char& ch : s) {
            if (ch == ' ') ch = '_';   // чтобы удобно было в Windows
        }
        return s;
    };

    while (true) {
        cout << "\nEnter bus name (example: 256) to generate SVG, or Q to exit: ";

        string bus_name;
        getline(cin >> ws, bus_name);  // важно: getline, чтобы работали имена с пробелами

        if (bus_name == "Q" || bus_name == "q") {
            cout << "Bye!\n";
            break;
        }

        const Bus* bus = catalogue.FindBus(bus_name);
        if (!bus) {
            cout << "Bus " << bus_name << ": not found\n";
            continue;
        }

        // Генерим SVG в файл
        const string filename = "bus_" + MakeSafeFilename(bus->name) + ".svg";
        ofstream out(filename);
        out << RenderBusSvg(*bus);
        out.close();

        cout << "SVG saved to: " << filename << "\n";
        cout << "Open it with a browser (double click in Explorer).\n";
    }
#endif

    return 0;
}
