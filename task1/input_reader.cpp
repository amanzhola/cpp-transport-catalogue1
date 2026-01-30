#include "input_reader.h"

#include <cmath>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

#ifdef TC_VERBOSE
#define TC_LOG(x) do { std::cerr << x << '\n'; } while(false)
#else
#define TC_LOG(x) do {} while(false)
#endif

std::string_view Trim(std::string_view s) {
    const std::size_t start = s.find_first_not_of(' ');
    if (start == s.npos) return {};
    const std::size_t end = s.find_last_not_of(' ');
    return s.substr(start, end + 1 - start);
}

std::vector<std::string_view> Split(std::string_view s, char delim) {
    std::vector<std::string_view> result;
    std::size_t pos = 0;

    while ((pos = s.find_first_not_of(' ', pos)) < s.size()) {
        std::size_t delim_pos = s.find(delim, pos);
        if (delim_pos == s.npos) delim_pos = s.size();

        std::string_view token = Trim(s.substr(pos, delim_pos - pos));
        if (!token.empty()) result.push_back(token);

        pos = delim_pos + 1;
    }
    return result;
}

tc::geo::Coordinates ParseCoordinates(std::string_view str) {
    const auto comma = str.find(',');
    if (comma == str.npos) {
        const double nan = std::nan("");
        return {nan, nan};
    }
    const std::string_view left  = Trim(str.substr(0, comma));
    const std::string_view right = Trim(str.substr(comma + 1));
    return {std::stod(std::string(left)), std::stod(std::string(right))};
}

struct ParsedRoute {
    std::vector<std::string_view> stops; // КАК ВО ВХОДЕ
    bool is_roundtrip = false;           // true для '>', false для '-'
};

ParsedRoute ParseRoute(std::string_view route) {
    ParsedRoute pr;
    if (route.find('>') != route.npos) {
        pr.is_roundtrip = true;
        pr.stops = Split(route, '>');
    } else {
        pr.is_roundtrip = false;
        pr.stops = Split(route, '-');
    }
    return pr;
}

tc::io::CommandDescription ParseCommandDescription(std::string_view line) {
    const std::size_t colon_pos = line.find(':');
    if (colon_pos == line.npos) return {};

    const std::size_t space_pos = line.find(' ');
    if (space_pos == line.npos || space_pos >= colon_pos) return {};

    const std::size_t id_start = line.find_first_not_of(' ', space_pos);
    if (id_start == line.npos || id_start >= colon_pos) return {};

    tc::io::CommandDescription cmd;
    cmd.command = std::string(line.substr(0, space_pos));
    cmd.id = std::string(line.substr(id_start, colon_pos - id_start));
    cmd.description = std::string(line.substr(colon_pos + 1));
    return cmd;
}

}  // namespace

namespace tc::io {

void InputReader::ParseLine(std::string_view line) {
    auto cmd = ParseCommandDescription(line);
    if (cmd) commands_.push_back(std::move(cmd));
}

void InputReader::ApplyCommands(tc::catalogue::TransportCatalogue& catalogue) const {
    TC_LOG("🧩 ApplyCommands: сначала Stops, потом Buses");

    // 1) Stops
    for (const auto& c : commands_) {
        if (c.command == "Stop") {
            const tc::geo::Coordinates coord = ParseCoordinates(c.description);
            TC_LOG("🛑 Stop: '" << c.id << "' -> (" << coord.lat << ", " << coord.lng << ")");
            catalogue.AddStop(c.id, coord);
        }
    }

    // 2) Buses
    for (const auto& c : commands_) {
        if (c.command == "Bus") {
            const auto pr = ParseRoute(c.description);
            TC_LOG("🚌 Bus: '" << c.id << "' stops=" << pr.stops.size()
                              << " round=" << (pr.is_roundtrip ? "YES" : "NO"));
            catalogue.AddBus(c.id, pr.stops, pr.is_roundtrip);
        }
    }
}

}  // namespace tc::io
