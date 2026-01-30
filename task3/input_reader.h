// input_reader.h
#pragma once

/**************************************************************************************************
 * ADDED:
 *   - namespace transport_catalogue::io
 *   - ApplyCommands принимает transport_catalogue::catalogue::TransportCatalogue&
 *   - В конце глобальные алиасы InputReader/CommandDescription
 *
 * ❌ REMOVED:
 *   - глобальные объявления структур и класса (теперь внутри io)
 **************************************************************************************************/

#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace transport_catalogue::io {

struct CommandDescription {
    explicit operator bool() const {
        return !command.empty();
    }
    bool operator!() const {
        return !operator bool();
    }

    std::string command;
    std::string id;
    std::string description;
};

class InputReader {
public:
    void ParseLine(std::string_view line);
    void ApplyCommands(transport_catalogue::catalogue::TransportCatalogue& catalogue) const;

private:
    std::vector<CommandDescription> commands_;
};

} // namespace transport_catalogue::io

// COMPAT: чтобы старые тесты видели эти имена
using CommandDescription = transport_catalogue::io::CommandDescription;
using InputReader = transport_catalogue::io::InputReader;
