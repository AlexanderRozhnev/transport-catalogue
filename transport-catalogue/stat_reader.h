#pragma once

/* Read requests for output. And output results */

#include <iostream>
#include <string_view>

#include "transport_catalogue.h"

namespace transport_catalogue {

enum class StatQueryType {
    Bus,
    Stop
};

struct StatQuery {
    StatQueryType type;
    std::string name;
};

class StatReader {
public:
    explicit StatReader(std::istream& input, std::ostream& output, TransportCatalogue catalogue);

private:
    std::istream& input_;
    std::ostream& output_;
    TransportCatalogue& catalogue_;

    StatQuery ParseStatQuery(std::string_view line);
    void AnswerBus(std::string_view name);
    void AnswerStop(std::string_view name);
};

} // namespace transport_catalogue