#include <iostream>
#include <string>

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

#ifdef TC_RENDER_MAP
#include "map_renderer.h"
#endif

static bool IsBlankLine(const std::string& s) {
    for (char c : s) {
        if (c != ' ' && c != '\t' && c != '\r') return false;
    }
    return true;
}

int main() {
    using namespace std;

    tc::catalogue::TransportCatalogue catalogue;
    tc::io::InputReader reader;

    int base_requests = 0;
    cin >> base_requests;

    string line;
    getline(cin, line); // съесть '\n' после числа

    // читаем ровно base_requests команд, пустые строки НЕ считаем
    for (int got = 0; got < base_requests && getline(cin, line); ) {
        if (IsBlankLine(line)) continue;
        reader.ParseLine(line);
        ++got;
    }

    reader.ApplyCommands(catalogue);

#ifdef TC_RENDER_MAP
    tc::render::RenderMapSvg(catalogue, cout);
    return 0;
#else
    int stat_requests = 0;
    cin >> stat_requests;
    getline(cin, line);

    tc::io::StatReader stat_reader(catalogue);

    for (int got = 0; got < stat_requests && getline(cin, line); ) {
        if (IsBlankLine(line)) continue;
        stat_reader.ParseAndPrintLine(line, cout);
        ++got;
    }
    return 0;
#endif
}
