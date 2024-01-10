#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>

#include "geo.h"

namespace domain {

/* bus stop. Contains: stop name, stop coordinates */
struct Stop {
    size_t id;
    std::string name;
    geo::Coordinates coordinates;
};

/* a type of bus route */
enum class BusType {
    UNKNOWN,
    LINEAR,
    CIRCULAR
};

/* bus route. Contains: bus name, all bus stops (aka bus route) */
struct Bus {
    size_t id;
    std::string name;
    BusType type = BusType::UNKNOWN;
    std::vector<const Stop*> stops;
};

/* bus info: name, stops count, unique stops count, geo route length, route length, curvuature.
   returned by GetBusInfo */
struct BusInfo {
    std::string name;
    int stops_count = 0;
    int unique_stops_count = 0;
    double geo_route_length = 0;
    int route_length = 0;
    double curvature = 1.0;
};

/* stop info: all buses names passes the stop.
   returned by GetStopInfo */
using StopInfo = std::unordered_set<std::string_view>;

/* hasher for unordered_map<pair<const Stop*, const Stop*> */
struct StopHasher {
    std::size_t operator() (const std::pair<const Stop*, const Stop*> ptr_stop) const {
        return hasher_(static_cast<const void*>(ptr_stop.first)) * 37
             + hasher_(static_cast<const void*>(ptr_stop.second));
    }
private:
    const std::hash<const void*> hasher_{};
};

} // namespace domain