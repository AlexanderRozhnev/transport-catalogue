#pragma once

/* Transport catalogue */

#include <string>
#include <string_view>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "geo.h"
#include "domain.h"

namespace transport_catalogue {
using namespace domain;

class TransportCatalogue {
public:
    /* Add Stop to database */
    void AddStop(const std::string& name, const geo::Coordinates& coordinates);

    /* Add Stop with specified id to database */
    void AddStop(size_t stop_id, const std::string& name, const geo::Coordinates& coordinates);

    /* Add Bus to database */
    void AddBus(const std::string& name, BusType type, const std::vector<std::string>& stop_names);

    /* Add Bus with specified id to database */
    void AddBus(size_t bus_id, const std::string& name, BusType type, const std::vector<std::string>& stop_names);

    /* Add Distance (between Stops) to database */
    void SetDistance(const std::string& from_stop, const std::string& to_stop, const int distance);

    /* Add Distance (between Stops) by stop ids to database */
    void SetDistance(size_t from_stop, size_t to_stop, const int distance);

    /* Search Stop by Name */
    const Stop* FindStop(const std::string_view name) const;

    /* Search Stop by id */
    const Stop* FindStop(size_t id) const;

    /* Search Bus by Name */
    const Bus* FindBus(const std::string_view name) const;

    /* Search Bus by id */
    const Bus* FindBus(size_t id) const;

    /* Get All Stops */
    const std::deque<Stop>& GetAllStops() const;

    /* Get All Routes */
    const std::deque<Bus>& GetAllBuses() const;

    /* Get distance between two Stops in meters (if set) */
    int GetDistance(const Stop* from, const Stop* to) const;

    /* Get distance between two Stops in meters (if set) */
    int GetDistance(const std::string& from_stop, const std::string& to_stop) const;

    /* Get distance between two Stops in meters (if set) */
    int GetDistance(const size_t from_stop_id, const size_t to_stop_id) const;

    /* Get Info about Stop */
    const StopInfo& GetStopInfo(const std::string_view name) const;

    /* Get Info about Bus (route) */
    const BusInfo GetBusInfo(const std::string_view name) const;

    /* Get Distances container */
    const std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopHasher>&
    GetDistances() const;

private:
    std::deque<Stop> stops_;                                            /* all bus stops for all buses */
    std::deque<Bus> buses_;                                             /* all buses routes */
    std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;/* fast access to stops by stop name */
    std::unordered_map<size_t, const Stop*> stop_id_to_stop_;/* fast access to stops by stop id */
    std::unordered_map<std::string_view, const Bus*> busname_to_bus_;   /* fast access to buses by bus name */
    std::unordered_map<size_t, const Bus*> bus_id_to_bus_;   /* fast access to buses by bus id */
    std::unordered_map<std::string_view, std::unordered_set<std::string_view>> stop_to_buses_; /* fast access to buses at the stop */
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopHasher> distances_; /* known real distances between stops */

    static size_t stop_id_;
    static size_t bus_id_;
};

inline size_t TransportCatalogue::stop_id_ = 0;
inline size_t TransportCatalogue::bus_id_ = 0;

namespace detail {

/* Delete leading and trailing whitespaces, but keep whitespaces inside */
std::string_view TrimLeadingAndTrailingSpaces(const std::string_view line);

/* split line into left and right parts by specified delimeter */
std::pair<std::string_view, std::string_view> SplitByFirst(const std::string_view line, const char delimeter);

/* split by delimeter and place parameters to vector<string> */
std::vector<std::string> SplitParameters(std::string_view line, char delimeter);

} // namespace detail

} // namespace transport_catalogue