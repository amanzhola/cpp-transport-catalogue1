#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace tc::io {

struct CommandDescription {
    std::string command;      // "Stop" или "Bus"
    std::string id;           // имя остановки / автобуса
    std::string description;  // всё после ':'

    explicit operator bool() const {
        return !command.empty();
    }
};

class InputReader {
public:
    void ParseLine(std::string_view line);
    void ApplyCommands(tc::catalogue::TransportCatalogue& catalogue) const;

private:
    std::vector<CommandDescription> commands_;
};

}  // namespace tc::io
