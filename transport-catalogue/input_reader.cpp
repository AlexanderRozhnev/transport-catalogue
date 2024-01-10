#include "input_reader.h"

#include <string_view>

namespace transport_catalogue {

using namespace std;

/* parse lines to queries */
InputQuery InputReader::ParseInputQuery(const string_view line) {

    using detail::SplitByFirst;
    using detail::SplitParameters;
    using detail::TrimLeadingAndTrailingSpaces;

    auto [header, values] = SplitByFirst(line, ':');
    auto [type, name] = SplitByFirst(header, ' ');
    
    /* detect query type */
    if (type.compare("Stop"sv) == 0) {                          
        /* Stop */
        return {InputQueryType::Stop, move(string{name}), SplitParameters(values, ',')};
    } else if (type.compare("Bus"sv) == 0) {                    
        /* Bus */
        if (values.find_first_of('>') != string_view::npos) {
            /* > > > circular route */
            return {InputQueryType::BusCircular, move(string{name}), SplitParameters(values, '>')};
        } else if (values.find_first_of('-') != string_view::npos) {
            /* - - - linear route */
            return {InputQueryType::BusLinear, move(string{name}), SplitParameters(values, '-')};
        } else {
            /* whithout delimeters only possible if bus has only 1 stop */
            return {InputQueryType::BusLinear, move(string{name}), vector<string>{string{TrimLeadingAndTrailingSpaces(values)}}};
        }
    }
    return {};
}

InputReader::InputReader(istream& input, TransportCatalogue& catalogue) : 
                                                    input_{input},
                                                    catalogue_{catalogue} {
    // Get count of requests
    int queries_count;
    input >> queries_count;
    string s;
    getline(input, s);

    // Get queries
    vector<InputQuery> queries;
    for (int i = 0; i < queries_count; ++i) {
        getline(input, s);
        InputQuery q = ParseInputQuery(s);
        if (q.type != InputQueryType::None) {
            queries.push_back(q);
        }
    }

    // 1st pass: Load Stops
    for (const auto& query : queries) {
        if (query.type == InputQueryType::Stop) {
            /* extract coordinates */
            double latitude = stod(query.parameters[0]);
            double longitude = stod(query.parameters[1]);
            catalogue.AddStop(query.name, geo::Coordinates{latitude, longitude});
        }
    }

    // 2nd pass: Load Buses
    for (auto& query : queries) {
        if (query.type == InputQueryType::BusCircular) {
            catalogue.AddBus(query.name, BusType::CIRCULAR, query.parameters);
        } else if (query.type == InputQueryType::BusLinear) {
            catalogue.AddBus(query.name, BusType::LINEAR, query.parameters);
        } else if (query.type == InputQueryType::Stop && query.parameters.size() > 2) {
            /* extract real distances */
            /* parametes has next format: "D1m to stop1, D2m to stop2, ..." */
            for (const string& param : query.parameters) {
                auto pos = param.find("m to ");
                if (pos != string::npos) {
                    int distance = stoi(param.substr(0, pos));
                    string other_stop = param.substr(pos - 1 + sizeof("m to "));
                    catalogue_.SetDistance(query.name, other_stop, distance); 
                }
            }
        }
    }
}

} // namespace transport_catalogue