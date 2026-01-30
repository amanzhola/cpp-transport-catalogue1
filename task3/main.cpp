// main.cpp
/**************************************************************************************************
 * TASK 3: Неймспейсы
 *
 * Требование: "одно namespace — для всего проекта за исключением функции main".
 * Поэтому main остаётся в глобальном пространстве имён.
 *
 * ADDED:
 *   - using для удобства: берём классы/функции из transport_catalogue::...
 *   - helper-функции main.cpp спрятаны в namespace detail (локально для main.cpp)
 *
 * ❌ REMOVED:
 *   - Ничего не переносили из main в общий namespace (это было бы против требования).
 **************************************************************************************************/

#include <iostream>
#include <string>

#include <fstream>
#include <cctype>
#include <iomanip>

#include "input_reader.h"
#include "stat_reader.h"

#ifdef INTERACTIVE
#include "map_renderer.h"
#include <algorithm>
#include <vector>
#include <tuple>
#endif

using namespace std;

// ADDED: локальные using — чтобы не писать длинные имена
using transport_catalogue::io::InputReader;
using transport_catalogue::stat::ParseAndPrintStat;
using transport_catalogue::catalogue::TransportCatalogue;

#ifdef INTERACTIVE
using transport_catalogue::render::RenderBusSvg;
using transport_catalogue::render::RenderStopSvg;
#endif

#ifdef INTERACTIVE
// ADDED: helper-ы main.cpp прячем в detail (локально, не в заголовке)
namespace detail {

// =============================================================
// Найти остановку, где сходятся 2 САМЫХ КОРОТКИХ маршрута
// =============================================================
static std::tuple<const Stop*, const Bus*, const Bus*, size_t>
FindStopWithTwoShortestIntersectingBuses(const TransportCatalogue& cat) {
    const auto& stops = cat.GetAllStops();

    const Stop* best_stop = nullptr;
    const Bus*  best_b1   = nullptr;
    const Bus*  best_b2   = nullptr;
    size_t best_score     = 0;

    for (const Stop* stop : stops) {
        const auto& buses_set = cat.GetBusesByStop(stop);

        if (buses_set.size() < 2) {
            continue;
        }

        const Bus* shortest1 = nullptr;
        const Bus* shortest2 = nullptr;

        for (const Bus* b : buses_set) {
            const size_t len = b->stops.size();

            if (!shortest1
                || len < shortest1->stops.size()
                || (len == shortest1->stops.size() && b->name < shortest1->name)) {
                shortest2 = shortest1;
                shortest1 = b;
            } else if (!shortest2
                       || len < shortest2->stops.size()
                       || (len == shortest2->stops.size() && b->name < shortest2->name)) {
                shortest2 = b;
            }
        }

        if (!shortest1 || !shortest2) {
            continue;
        }

        const size_t score = shortest1->stops.size() + shortest2->stops.size();

        if (!best_stop
            || score < best_score
            || (score == best_score && stop->name < best_stop->name)) {
            best_stop = stop;
            best_b1 = shortest1;
            best_b2 = shortest2;
            best_score = score;
        }
    }

    return {best_stop, best_b1, best_b2, best_score};
}

// Вставляем строку "Score" в SVG
static std::string InjectSummaryIntoSvg(std::string svg, size_t score_sum) {
    const double x = 20.0;
    const double y = 120.0;

    std::ostringstream add;
    add << "\n  <!-- Score injected by main.cpp -->\n";

    add << "  <rect x=\"" << (x - 10) << "\" y=\"" << (y - 20)
        << "\" width=\"320\" height=\"34\" fill=\"white\" opacity=\"0.85\" />\n";

    add << "  <text x=\"" << x << "\" y=\"" << y
        << "\" font-family=\"Verdana\" font-size=\"16\" fill=\"black\">"
        << "Score (sum): " << score_sum
        << "</text>\n";

    const std::string close = "</svg>";
    const auto pos = svg.rfind(close);
    if (pos == std::string::npos) {
        svg += add.str();
        return svg;
    }
    svg.insert(pos, add.str());
    return svg;
}

} // namespace detail
#endif

int main(int argc, char** argv) {
    TransportCatalogue catalogue;

    istream* input = &cin;
    ifstream fin;

#ifdef INTERACTIVE
    if (argc >= 2) {
        fin.open(argv[1]);
        if (!fin) {
            cout << "Cannot open file: " << argv[1] << "\n";
            return 1;
        }
        input = &fin;
    }
#endif

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
    int stat_request_count = 0;
    (*input) >> stat_request_count >> ws;

    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(*input, line);
        ParseAndPrintStat(catalogue, line, cout);
    }
#else
    const auto& buses = catalogue.GetAllBuses();
    const auto& stops = catalogue.GetAllStops();

    cout << "Found routes: " << buses.size() << "\n\n";
    if (buses.empty()) {
        cout << "No routes to show.\n";
        return 0;
    }

    for (size_t i = 0; i < buses.size(); ++i) {
        const Bus* bus = buses[i];
        const auto stat = catalogue.GetBusStat(bus->name);

        cout << (i + 1) << ") "
             << "Bus " << bus->name << ": "
             << stat.stops_count << " stops on route, "
             << stat.unique_stops << " unique stops, "
             << setprecision(6) << stat.route_length << " route length\n";
    }

    cout << "\nStops list: " << stops.size() << "\n\n";
    for (size_t i = 0; i < stops.size(); ++i) {
        const Stop* stop = stops[i];
        const auto& buses_set = catalogue.GetBusesByStop(stop);

        cout << (i + 1) << ") Stop " << stop->name
             << " (" << buses_set.size() << " routes)\n";
    }

    {
        auto [st, b1, b2, score] = detail::FindStopWithTwoShortestIntersectingBuses(catalogue);

        if (!st) {
            cout << "\nNo stop with >=2 routes found (no intersections).\n";
        } else {
            cout << "\n✅ Two shortest routes that intersect at one stop:\n";
            cout << "Stop: " << st->name << "\n";
            cout << "Bus " << b1->name << " (" << b1->stops.size() << " stops)\n";
            cout << "Bus " << b2->name << " (" << b2->stops.size() << " stops)\n";
            cout << "Score (sum): " << score << "\n";

            vector<const Bus*> best_vec = {b1, b2};

            sort(best_vec.begin(), best_vec.end(),
                 [](const Bus* a, const Bus* b) { return a->name < b->name; });

            string svg = RenderStopSvg(*st, best_vec);
            svg = detail::InjectSummaryIntoSvg(std::move(svg), score);

            const string filename = "best_intersection_stop.svg";
            ofstream out(filename);
            out << svg;
            out.close();

            cout << "SVG saved to: " << filename << "\n";
        }
    }

    auto Trim = [](string s) {
        while (!s.empty() && isspace(static_cast<unsigned char>(s.front()))) {
            s.erase(s.begin());
        }
        while (!s.empty() && isspace(static_cast<unsigned char>(s.back()))) {
            s.pop_back();
        }
        return s;
    };

    auto MakeSafeFilename = [](string s) {
        for (char& ch : s) {
            if (ch == ' ') ch = '_';
        }
        return s;
    };

    while (true) {
        cout << "\nCommands:\n"
             << "  B <number>  - render route by index (e.g. B 1)\n"
             << "  S <number>  - render stop  by index with all its routes (e.g. S 3)\n"
             << "  <bus_name>  - render route by name (e.g. 256)\n"
             << "  Q           - exit\n"
             << "Enter command: ";

        string line;
        getline(cin >> ws, line);
        line = Trim(line);

        if (line == "Q" || line == "q") {
            cout << "Bye!\n";
            break;
        }

        if (line.size() >= 2 &&
            (line[0] == 'B' || line[0] == 'b' || line[0] == 'S' || line[0] == 's') &&
            isspace(static_cast<unsigned char>(line[1]))) {

            char cmd = static_cast<char>(toupper(static_cast<unsigned char>(line[0])));
            string num_str = Trim(line.substr(1));

            size_t idx = 0;
            try {
                idx = static_cast<size_t>(stoul(num_str));
            } catch (...) {
                cout << "Bad number: " << num_str << "\n";
                continue;
            }

            if (cmd == 'B') {
                const Bus* bus = catalogue.GetBusByIndex(idx);
                if (!bus) {
                    cout << "Bus #" << idx << ": not found\n";
                    continue;
                }

                const string filename = "bus_" + MakeSafeFilename(bus->name) + ".svg";
                ofstream out(filename);
                out << RenderBusSvg(*bus);
                out.close();

                cout << "SVG saved to: " << filename << "\n";
                cout << "Open it with a browser.\n";
                continue;
            }

            if (cmd == 'S') {
                const Stop* stop = catalogue.GetStopByIndex(idx);
                if (!stop) {
                    cout << "Stop #" << idx << ": not found\n";
                    continue;
                }

                const auto& buses_set = catalogue.GetBusesByStop(stop);
                if (buses_set.empty()) {
                    cout << "Stop " << stop->name << ": no buses\n";
                    continue;
                }

                vector<const Bus*> buses_vec(buses_set.begin(), buses_set.end());
                sort(buses_vec.begin(), buses_vec.end(),
                     [](const Bus* a, const Bus* b) { return a->name < b->name; });

                constexpr size_t kMaxRoutesInStopSvg = 2;
                if (buses_vec.size() > kMaxRoutesInStopSvg) {
                    cout << "Stop " << stop->name << ": showing only first "
                         << kMaxRoutesInStopSvg << " routes out of " << buses_vec.size()
                         << " in SVG\n";
                    buses_vec.resize(kMaxRoutesInStopSvg);
                }

                const string filename = "stop_" + MakeSafeFilename(stop->name) + ".svg";
                ofstream out(filename);
                out << RenderStopSvg(*stop, buses_vec);
                out.close();

                cout << "SVG saved to: " << filename << "\n";
                cout << "Open it with a browser.\n";
                continue;
            }
        }

        const Bus* bus = catalogue.FindBus(line);
        if (!bus) {
            cout << "Bus " << line << ": not found\n";
            continue;
        }

        const string filename = "bus_" + MakeSafeFilename(bus->name) + ".svg";
        ofstream out(filename);
        out << RenderBusSvg(*bus);
        out.close();

        cout << "SVG saved to: " << filename << "\n";
        cout << "Open it with a browser.\n";
    }
#endif

    return 0;
}
