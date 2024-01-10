#include "transport_catalogue.h"

#include <unordered_set>
#include <functional>

namespace transport_catalogue {

using namespace std;

void TransportCatalogue::AddStop
            (const string& name, const geo::Coordinates& coordinates) {
    stop_id_ = std::max(stop_id_, stops_.size());
    AddStop(stop_id_, name, coordinates);
    ++stop_id_;
}

void TransportCatalogue::AddStop
            (size_t stop_id, const std::string &name, const geo::Coordinates &coordinates) {
    Stop& stop = stops_.emplace_back(Stop{stop_id, name, coordinates});
    stopname_to_stop_[string_view{stop.name}] = &stop;
    stop_id_to_stop_[stop_id] = &stop;
}

void TransportCatalogue::AddBus
            (const string& name, BusType type, const vector<string>& stop_names) {
    bus_id_ = std::max(bus_id_, buses_.size());
    AddBus(bus_id_, name, type, stop_names);
    ++bus_id_;
}

void TransportCatalogue::AddBus
            (size_t bus_id, const string& name, BusType type, const vector<string>& stop_names) {
    vector<const Stop*> stops;
    for (const string& stopname : stop_names) {
        stops.push_back(stopname_to_stop_.at(stopname));
    }
    Bus& bus = buses_.emplace_back(Bus{bus_id, name, type, move(stops)});
    busname_to_bus_[string_view{bus.name}] = &bus;
    bus_id_to_bus_[bus_id] = &bus;

    /* add bus to all stops */
    for (const auto stop : bus.stops) {
        stop_to_buses_[stop->name].insert(string_view(bus.name));
    }
}

void TransportCatalogue::SetDistance
            (const string& from_stop, const string& to_stop, const int distance) {
    const Stop* from = stopname_to_stop_.at(from_stop);
    const Stop* to = stopname_to_stop_.at(to_stop);
    distances_[pair{from, to}] = distance;
}

void TransportCatalogue::SetDistance(size_t from_stop, size_t to_stop, const int distance) {
    const Stop* from = stop_id_to_stop_.at(from_stop);
    const Stop* to = stop_id_to_stop_.at(to_stop);
    distances_[pair{from, to}] = distance;
}

const Stop* TransportCatalogue::FindStop(const string_view name) const {
    auto it = stopname_to_stop_.find(name);
    if (it != stopname_to_stop_.end()) {
        return it->second;
    }
    return nullptr;
}

const Stop* TransportCatalogue::FindStop(size_t id) const {
    auto it = stop_id_to_stop_.find(id);
    if (it != stop_id_to_stop_.end()) {
        return it->second;
    }    
    return nullptr;
}

const Bus* TransportCatalogue::FindBus(const string_view name) const {
    auto it = busname_to_bus_.find(name);
    if (it != busname_to_bus_.end()) {
        return it->second;
    }
    return nullptr;
}

const Bus* TransportCatalogue::FindBus(size_t id) const {
    auto it = bus_id_to_bus_.find(id);
    if (it != bus_id_to_bus_.end()) {
        return it->second;
    }
    return nullptr;
}

const std::deque<Stop>& TransportCatalogue::GetAllStops() const {
    return stops_;
}

const std::deque<Bus>& TransportCatalogue::GetAllBuses() const {
    return buses_;
}

int TransportCatalogue::GetDistance(const Stop* from, const Stop* to) const {
    if (!from || !to) return 0;

    if (distances_.count(pair{from, to}) != 0) {
        return distances_.at(pair{from, to});
    } else if (distances_.count(pair{to, from}) != 0) {
        return distances_.at(pair{to, from});
    }
    return 0;
}

int TransportCatalogue::GetDistance(const string& from_stop, const string& to_stop) const {
    const Stop* from = FindStop(from_stop);
    const Stop* to = FindStop(to_stop);
    return GetDistance(from, to);
}

int TransportCatalogue::GetDistance(const size_t from_stop_id, const size_t to_stop_id) const {
    const Stop* from = FindStop(from_stop_id);
    const Stop* to = FindStop(to_stop_id);
    return GetDistance(from, to);
}

const BusInfo
TransportCatalogue::GetBusInfo(const string_view name) const {
    const Bus* bus = FindBus(name);
    if (!bus) return {};

    double geo_route_length = 0.0;
    int route_length = 0;
    double curvature = 1.0;
    int unique_stops_count = 0;
    int stop_count = static_cast<int>(bus->stops.size());
    if (stop_count > 0) {
        unordered_set<string_view> unique_stops;
        unique_stops.insert(bus->stops.front()->name);

        /* bus goes forward */
        for (auto it1 = bus->stops.begin(), it2 = it1 + 1; it2 != bus->stops.end(); ++it1, ++it2) {
            geo_route_length += geo::ComputeDistance((*it1)->coordinates, (*it2)->coordinates);
            const int& distance = GetDistance((*it1)->name, (*it2)->name);
            route_length += (distance != 0) ? distance : static_cast<int>(geo_route_length);
            unique_stops.insert((*it2)->name);
        }

        /* bus goes backward if route linear */
        if (bus->type == BusType::LINEAR) {
            for (auto it1 = bus->stops.rbegin(), it2 = it1 + 1; it2 != bus->stops.rend(); ++it1, ++it2) {
                geo_route_length += geo::ComputeDistance((*it1)->coordinates, (*it2)->coordinates);
                const int& distance = GetDistance((*it1)->name, (*it2)->name);
                route_length += (distance != 0) ? distance : static_cast<int>(geo_route_length);
            }
            
            /* unique case than only 2 stops and 1st stop == last stop */
            if (stop_count == 2 && bus->stops.front() == bus->stops.back()) {
                stop_count = 1;    
            } else {
                stop_count += static_cast<int>(bus->stops.size()) - 1;
            }
        }
        
        unique_stops_count = static_cast<int>(unique_stops.size());
        if (geo_route_length > 0) curvature = route_length / geo_route_length;
    }
    return {string{bus->name}, stop_count, unique_stops_count, geo_route_length, route_length, curvature};
}

const std::unordered_map<std::pair<const Stop *, const Stop *>, int, StopHasher>&
TransportCatalogue::GetDistances() const {
    return distances_;
}

/* get all buses names passes the stop. all buses names unique and sorted by name */
const StopInfo&
TransportCatalogue::GetStopInfo(const string_view stop_name) const {
    static const StopInfo empty{};

    auto stop_ptr = stop_to_buses_.find(stop_name);
    if (stop_ptr != stop_to_buses_.end()) {
        return std::cref(stop_ptr->second);
    }

    return empty;
}

namespace detail {
    
string_view TrimLeadingAndTrailingSpaces(const string_view line) {
    // const string whitespaces{" \t\f\v\n\r"};
    const string whitespaces{" "};

    size_t start_pos = line.find_first_not_of(whitespaces);     /* skip leading whitespaces */
    if (start_pos != string_view::npos) {
        size_t last_pos = line.find_last_not_of(whitespaces);   /* skip trailing whitespaces */
        if (last_pos != string_view::npos) {
            return line.substr(start_pos, last_pos - start_pos + 1);
        }
    }
    return string_view{};   /* whole string consist of whitespaces */
}

pair<string_view, string_view> SplitByFirst(const string_view line, const char delimeter) {
    size_t pos = line.find(delimeter);
    string_view left = line.substr(0, pos);                             /* delimeter not included */
    if (pos != string_view::npos && pos + 1 < line.size()) {
        string_view right = line.substr(pos + 1, string_view::npos);    /* delimeter not included */
        return pair{TrimLeadingAndTrailingSpaces(left), TrimLeadingAndTrailingSpaces(right)};
    }
    return pair{TrimLeadingAndTrailingSpaces(left), string_view{}};
}

vector<string> SplitParameters(string_view line, char delimeter) {
    vector<string> result;
    while (!line.empty()) {
        auto [value, tail] = SplitByFirst(line, delimeter);
        result.push_back(string{value});
        line = tail;
    }
    return result;
}

} // namespace detail

} // namespace transport_catalogue