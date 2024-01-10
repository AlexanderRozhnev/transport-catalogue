#include "request_handler.h"

namespace transport_catalogue {

using namespace std;

RequestHandler::RequestHandler(TransportCatalogue& db) : db_(db) {
}

void RequestHandler::AddStop(const std::string &name, const geo::Coordinates &coordinates) {
    db_.AddStop(name, coordinates);
}

void RequestHandler::AddBus(const std::string &name, BusType type, const std::vector<std::string> &stops) {
    db_.AddBus(name, type, stops);
}

void RequestHandler::SetDistance(const std::string &from_stop, const std::string &to_stop, const int distance) {
    db_.SetDistance(from_stop, to_stop, distance);
}

optional<const BusInfo> RequestHandler::GetBusStat(const string_view& bus_name) const {
    const Bus* bus = db_.FindBus(bus_name);
    if (bus) {
        const BusInfo& info = db_.GetBusInfo(bus_name);
        return optional<const BusInfo>{info};
    }
    return nullopt;
}

const Stop* RequestHandler::FindStop(const std::string_view name) const {
    return db_.FindStop(name);
}

const Stop* RequestHandler::FindStop(const size_t id) const {
    return db_.FindStop(id);
}

const Bus* RequestHandler::FindBus(const std::string_view name) const {
    return db_.FindBus(name);
}

const Bus* RequestHandler::FindBus(const size_t id) const {
    return db_.FindBus(id);
}

optional<const StopInfo> RequestHandler::GetBusesByStop(const string_view& stop_name) const {
    const Stop* stop = db_.FindStop(stop_name);
    if (stop) {
        const StopInfo& info = db_.GetStopInfo(stop_name);
        return optional<const StopInfo>{info};
    }
    return nullopt;
}

const std::deque<Stop>& RequestHandler::GetAllStops() const {
    return db_.GetAllStops();
}

const std::deque<Bus>& RequestHandler::GetAllBuses() const {
    return db_.GetAllBuses();
}

int RequestHandler::GetDistance(const std::string& from_stop, const std::string& to_stop) const {
    return db_.GetDistance(from_stop, to_stop);
}

int RequestHandler::GetDistance(const size_t from_stop_id, const size_t to_stop_id) const {
    return db_.GetDistance(from_stop_id, to_stop_id);
}

} // namespace transport_catalogue