#pragma once

/* Read queries for initial input to database */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "transport_catalogue.h"

namespace transport_catalogue {

enum class InputQueryType {
    None,
    BusCircular,
    BusLinear,
    Stop
};

struct InputQuery {
    InputQueryType type = InputQueryType::None;
    std::string name;
    std::vector<std::string> parameters;
};

class InputReader {
public:
    explicit InputReader(std::istream& input, TransportCatalogue& catalogue);

private:
    std::istream& input_;
    TransportCatalogue& catalogue_;

    InputQuery ParseInputQuery(const std::string_view line);
};

} // namespace transport_catalogue