#pragma once

#include <optional>
#include <deque>

#include "transport_catalogue.h"

/*
 * Код обработчика запросов к базе
 */

namespace transport_catalogue {
    
class RequestHandler {

public:
    /* ctor with Map renderer */
    explicit RequestHandler(TransportCatalogue& db);

    /* Add Stop to database */
    void AddStop(const std::string& name, const geo::Coordinates& coordinates);

    /* Add Route to database */
    void AddBus(const std::string& name, BusType type, const std::vector<std::string>& stops);

    /* Add Distance (between Stops) to database */
    void SetDistance(const std::string& from_stop, const std::string& to_stop, const int distance);

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<const BusInfo> GetBusStat(const std::string_view& bus_name) const;
    
    /* Search Stop by Name */
    const Stop* FindStop(const std::string_view name) const;

    /* Search Stop by Id */
    const Stop* FindStop(const size_t id) const;

    /* Search Bus by Name */
    const Bus* FindBus(const std::string_view name) const;

    /* Search Bus by Name */
    const Bus* FindBus(const size_t id) const;

    // Возвращает маршруты, проходящие через остановку
    std::optional<const StopInfo> GetBusesByStop(const std::string_view& stop_name) const;

    // Get All Stops
    const std::deque<Stop>& GetAllStops() const;

    // Get All Buses (all routes)
    const std::deque<Bus>& GetAllBuses() const;

    /* Get distance between two Stops in meters (if set) */
    int GetDistance(const std::string& from_stop, const std::string& to_stop) const;

    /* Get distance between two Stops in meters (if set) */
    int GetDistance(const size_t from_stop_id, const size_t to_stop_id) const;

private:
    TransportCatalogue& db_;              // RequestHandler agregates TransportCatalogue and MapRenderer
};

} // namespace transport_catalogue