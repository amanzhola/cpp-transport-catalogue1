// main.cpp for task2 (обычный режим + интерактивный SVG-режим)
//
// Сборка обычная:
//   g++ -std=c++17 -O2 -Wall -Wextra -pedantic ^
//     main.cpp transport_catalogue.cpp input_reader.cpp stat_reader.cpp ^
//     -o transport_catalogue.exe
// Запуск обычный:
//   transport_catalogue.exe < input.txt > output.txt
//
// Сборка интерактивная (SVG):
//   g++ -std=c++17 -O2 -Wall -Wextra -pedantic ^
//     -DINTERACTIVE ^
//     main.cpp transport_catalogue.cpp input_reader.cpp stat_reader.cpp map_renderer.cpp ^
//     -o transport_catalogue.exe
// Запуск интерактивный:
//   transport_catalogue.exe input.txt
//
// Что нового для Task2:
// 1) В обычном режиме (без INTERACTIVE) теперь поддерживаются stat-запросы:
//    - Bus X
//    - Stop X
//
// 2) В интерактивном режиме (INTERACTIVE) после списка маршрутов печатается
//    второй список: остановки с номерами и количеством маршрутов через них.
//    Далее можно:
//    - B <номер>  -> SVG одного маршрута (как раньше, но по номеру)
//    - S <номер>  -> SVG остановки со ВСЕМИ маршрутами через неё (разные цвета)
//    - <имя автобуса> -> SVG маршрута по имени (как раньше)
//    - Q -> выход

#include <iostream>
#include <string>

#include <fstream>   // ifstream/ofstream
#include <cctype>    // isspace, toupper
#include <iomanip>   // setprecision

#include "input_reader.h"
#include "stat_reader.h"

#ifdef INTERACTIVE
#include "map_renderer.h"   // RenderBusSvg, RenderStopSvg
#include <algorithm>        // sort
#include <vector>           // vector

#include <tuple>   // для std::tuple и structured bindings

#endif

using namespace std;

#ifdef INTERACTIVE
// =============================================================
// Найти остановку, где сходятся 2 САМЫХ КОРОТКИХ маршрута
// (критерий: минимальная сумма количества остановок этих 2 маршрутов).
//
// Возвращает:
//   stop  - общая остановка
//   b1,b2 - два маршрута, проходящие через неё
//   score - len(b1)+len(b2)
// Если подходящих остановок нет (нет ни одной остановки с >=2 маршрутами) -> stop=nullptr.
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

        // Чтобы "сходились" — нужно минимум 2 маршрута на одной остановке
        if (buses_set.size() < 2) {
            continue;
        }

        // Ищем два самых коротких маршрута из buses_set
        const Bus* shortest1 = nullptr;
        const Bus* shortest2 = nullptr;

        for (const Bus* b : buses_set) {
            const size_t len = b->stops.size();

            // вставка в (shortest1, shortest2) как в "топ-2 минимумов"
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
#endif

#ifdef INTERACTIVE
// Вставляем ОДНУ строку "Score" в SVG, не дублируя Stop/Bus,
// потому что Stop и Bus уже рисуются внутри RenderStopSvg.
static std::string InjectSummaryIntoSvg(std::string svg,
                                        size_t score_sum) {
    // Ставим текст ниже зоны заголовка+легенды RenderStopSvg
    const double x = 20.0;
    const double y = 120.0;

    std::ostringstream add;
    add << "\n  <!-- Score injected by main.cpp -->\n";

    // Белая подложка, чтобы текст читался поверх линий
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
#endif

int main(int argc, char** argv) {
    // ------------------------------------------------------------
    // 1) Создаём транспортный каталог — ядро программы:
    //    - хранит Stops/Bus (внутри deque)
    //    - индексы по именам
    //    - индекс stop -> set(bus) для Stop-статистики
    // ------------------------------------------------------------
    TransportCatalogue catalogue;

    // ------------------------------------------------------------
    // 2) Источник ввода базы (Stop/Bus для наполнения):
    //    - по умолчанию читаем из stdin
    //    - в INTERACTIVE режиме можно читать базу из файла argv[1]
    // ------------------------------------------------------------
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

    // ------------------------------------------------------------
    // 3) Читаем количество базовых запросов и сами запросы базы.
    //    Эти строки содержат:
    //      Stop ...  (координаты)
    //      Bus  ...  (маршрут через остановки)
    //    InputReader собирает их в память и потом применяет:
    //      1) сначала AddStop для всех остановок
    //      2) затем AddBus  для всех маршрутов
    // ------------------------------------------------------------
    int base_request_count = 0;
    (*input) >> base_request_count >> ws; // ws съедает пробелы/переводы строки после числа

    {
        InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            string line;
            getline(*input, line);      // читаем строку целиком (включая пробелы в названиях)
            reader.ParseLine(line);     // разбираем на CommandDescription
        }
        reader.ApplyCommands(catalogue); // реально добавляем данные в каталог
    }

#ifndef INTERACTIVE
    // ============================================================
    // 4A) Обычный режим (как в задании):
    //     После базы читаем stat-запросы и печатаем ответы.
    //     В task2 поддерживаем:
    //       - "Bus X"
    //       - "Stop X"
    // ============================================================
    int stat_request_count = 0;
    (*input) >> stat_request_count >> ws;

    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(*input, line);
        ParseAndPrintStat(catalogue, line, cout);
    }

#else
    // ============================================================
    // 4B) Интерактивный режим (SVG):
    //
    //     Мы НЕ читаем stat-запросы как в задании.
    //     Вместо этого:
    //       1) печатаем список маршрутов (Bus) с номерами (1..N)
    //       2) печатаем список остановок (Stop) с номерами (1..M)
    //          и показываем, сколько маршрутов через каждую проходит
    //       3) принимаем команды пользователя и генерим SVG-файлы
    // ============================================================

    // ---- Получаем "порядки" из каталога ----
    // buses:  в порядке добавления Bus (для интерактива и списка)
    // stops:  в порядке добавления Stop (для интерактива и списка)
    const auto& buses = catalogue.GetAllBuses();
    const auto& stops = catalogue.GetAllStops();  // <-- ВАЖНО: тут должна быть ; в конце

    // ---- Список маршрутов ----
    cout << "Found routes: " << buses.size() << "\n\n";

    // Если маршрутов нет — смысла генерить svg маршрутов/остановок мало
    if (buses.empty()) {
        cout << "No routes to show.\n";
        return 0;
    }

    // Печатаем список маршрутов в формате как в задании + порядковый номер
    // Номер — это индекс в buses (1..N), по нему можно вызвать B <номер>.
    for (size_t i = 0; i < buses.size(); ++i) {
        const Bus* bus = buses[i];
        const auto stat = catalogue.GetBusStat(bus->name);

        cout << (i + 1) << ") "
             << "Bus " << bus->name << ": "
             << stat.stops_count << " stops on route, "
             << stat.unique_stops << " unique stops, "
             << setprecision(6) << stat.route_length << " route length\n";
    }

    // ---- Список остановок ----
    // Для каждой остановки считаем, сколько маршрутов через неё проходит:
    // catalogue.GetBusesByStop(stop) -> unordered_set<const Bus*>
    cout << "\nStops list: " << stops.size() << "\n\n";
    for (size_t i = 0; i < stops.size(); ++i) {
        const Stop* stop = stops[i];
        const auto& buses_set = catalogue.GetBusesByStop(stop);

        cout << (i + 1) << ") Stop " << stop->name
             << " (" << buses_set.size() << " routes)\n";
    }


    // ------------------------------------------------------------
    // 4B-EXTRA) Находим "пересечение": 2 самых коротких маршрута,
    // которые сходятся на одной остановке.
    // ------------------------------------------------------------
    {
        auto [st, b1, b2, score] = FindStopWithTwoShortestIntersectingBuses(catalogue);

        if (!st) {
            cout << "\nNo stop with >=2 routes found (no intersections).\n";
        } else {
            cout << "\n✅ Two shortest routes that intersect at one stop:\n";
            cout << "Stop: " << st->name << "\n";
            cout << "Bus " << b1->name << " (" << b1->stops.size() << " stops)\n";
            cout << "Bus " << b2->name << " (" << b2->stops.size() << " stops)\n";
            cout << "Score (sum): " << score << "\n";

            // ------------------------------------------------------------
            // ✅ Рисуем SVG для найденного пересечения:
            //    - фокус: stop st
            //    - маршруты: b1 и b2 (ровно два)
            //    - в SVG добавляем текстовую сводку (Stop/Bus/Score)
            // ------------------------------------------------------------
            vector<const Bus*> best_vec = {b1, b2};

            // для стабильности цветов/легенды можно отсортировать по имени:
            sort(best_vec.begin(), best_vec.end(),
                 [](const Bus* a, const Bus* b) { return a->name < b->name; });

            // Рендерим SVG "остановка + эти 2 маршрута"
            string svg = RenderStopSvg(*st, best_vec);

            // Добавляем текстовую сводку внутрь SVG (не в map_renderer.cpp, а здесь)
            svg = InjectSummaryIntoSvg(std::move(svg), score);

            // Сохраняем результат
            const string filename = "best_intersection_stop.svg";
            ofstream out(filename);
            out << svg;
            out.close();

            cout << "SVG saved to: " << filename << "\n";
        }
    }

    // ------------------------------------------------------------
    // 5) Вспомогательные лямбды
    // ------------------------------------------------------------

    // Trim: убирает пробелы по краям строки (полезно для ввода команд)
    auto Trim = [](string s) {
        while (!s.empty() && isspace(static_cast<unsigned char>(s.front()))) {
            s.erase(s.begin());
        }
        while (!s.empty() && isspace(static_cast<unsigned char>(s.back()))) {
            s.pop_back();
        }
        return s;
    };

    // MakeSafeFilename: делаем имя файла безопаснее для Windows (пробелы -> _)
    auto MakeSafeFilename = [](string s) {
        for (char& ch : s) {
            if (ch == ' ') ch = '_';
        }
        return s;
    };

    // ------------------------------------------------------------
    // 6) Цикл команд пользователя
    //
    // Команды:
    //   B <номер>    -> берём bus по порядковому номеру и делаем bus_*.svg
    //   S <номер>    -> берём stop по номеру и делаем stop_*.svg
    //                  (показываем все маршруты через эту остановку)
    //   <bus_name>   -> старый вариант: ввод имени автобуса (например "256")
    //   Q            -> выход
    // ------------------------------------------------------------
    while (true) {
        cout << "\nCommands:\n"
             << "  B <number>  - render route by index (e.g. B 1)\n"
             << "  S <number>  - render stop  by index with all its routes (e.g. S 3)\n"
             << "  <bus_name>  - render route by name (e.g. 256)\n"
             << "  Q           - exit\n"
             << "Enter command: ";

        string line;
        getline(cin >> ws, line);    // читаем всю строку (ws съест leading whitespace)
        line = Trim(line);

        // ---- Выход ----
        if (line == "Q" || line == "q") {
            cout << "Bye!\n";
            break;
        }

        // --------------------------------------------------------
        // A) Команды вида "B 12" / "S 5"
        // Проверяем:
        //  - минимум 2 символа
        //  - первый символ B/b или S/s
        //  - второй символ пробельный (значит дальше число)
        // --------------------------------------------------------
        if (line.size() >= 2 &&
            (line[0] == 'B' || line[0] == 'b' || line[0] == 'S' || line[0] == 's') &&
            isspace(static_cast<unsigned char>(line[1]))) {

            char cmd = static_cast<char>(toupper(static_cast<unsigned char>(line[0])));
            string num_str = Trim(line.substr(1)); // всё после буквы

            size_t idx = 0;
            try {
                idx = static_cast<size_t>(stoul(num_str));
            } catch (...) {
                cout << "Bad number: " << num_str << "\n";
                continue;
            }

            // ---------- B <номер>: SVG маршрута по порядку ----------
            if (cmd == 'B') {
                // GetBusByIndex ожидает 1..N, иначе nullptr
                const Bus* bus = catalogue.GetBusByIndex(idx);
                if (!bus) {
                    cout << "Bus #" << idx << ": not found\n";
                    continue;
                }

                // Генерим SVG маршрута в файл
                const string filename = "bus_" + MakeSafeFilename(bus->name) + ".svg";
                ofstream out(filename);
                out << RenderBusSvg(*bus);
                out.close();

                cout << "SVG saved to: " << filename << "\n";
                cout << "Open it with a browser.\n";
                continue;
            }

            // ---------- S <номер>: SVG остановки со всеми маршрутами ----------(изменил на 2 только временно)
	    // ---------- S <номер>: SVG остановки (показываем ТОЛЬКО первые 2 маршрута) ----------
            if (cmd == 'S') {
                // GetStopByIndex ожидает 1..M, иначе nullptr
                const Stop* stop = catalogue.GetStopByIndex(idx);
                if (!stop) {
                    cout << "Stop #" << idx << ": not found\n";
                    continue;
                }

                // Получаем все маршруты через эту остановку
                const auto& buses_set = catalogue.GetBusesByStop(stop);

                // Если маршрутов нет — сообщаем и не создаём svg
                if (buses_set.empty()) {
                    cout << "Stop " << stop->name << ": no buses\n";
                    continue;
                }

                // unordered_set не гарантирует порядок.
                // Для стабильных цветов/легенды делаем vector и сортируем по имени маршрута.
                vector<const Bus*> buses_vec(buses_set.begin(), buses_set.end());
                sort(buses_vec.begin(), buses_vec.end(),
                     [](const Bus* a, const Bus* b) { return a->name < b->name; });

		// ------------------------------------------------------------------
		// ⚠ Ограничение для SVG по остановке:
		// Чтобы SVG не был перегружен, показываем ТОЛЬКО первые 2 маршрута
		// (после сортировки по имени, чтобы выбор был стабильным).
		//
		// Если маршрутов больше двух, остальные мы НЕ рисуем в SVG-файле.
		// В консоли при этом можно сообщить пользователю, что было скрыто.
		// ------------------------------------------------------------------
		constexpr size_t kMaxRoutesInStopSvg = 2;

		if (buses_vec.size() > kMaxRoutesInStopSvg) {
    		   cout << "Stop " << stop->name << ": showing only first "
         		<< kMaxRoutesInStopSvg << " routes out of " << buses_vec.size()
         		<< " in SVG\n";

    		   buses_vec.resize(kMaxRoutesInStopSvg); // оставляем только первые 2
		}
		
		// Генерим SVG остановки (внутри SVG будут все эти маршруты разными цветами)
                const string filename = "stop_" + MakeSafeFilename(stop->name) + ".svg";
                ofstream out(filename);
                out << RenderStopSvg(*stop, buses_vec);
                out.close();

                cout << "SVG saved to: " << filename << "\n";
                cout << "Open it with a browser.\n";
                continue;
            }
        }

        // --------------------------------------------------------
        // B) Старый режим: пользователь ввёл имя автобуса (например "256")
        // --------------------------------------------------------
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
#endif  // INTERACTIVE

    return 0;
}
