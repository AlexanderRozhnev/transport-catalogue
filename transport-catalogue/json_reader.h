#pragma once

/* Load transport catalogue data from JSON
 * Read status requests to transpot catalogue from JSON
 * Proccess status requests to transpot catalogue through RequestHandler
 * Get answers from RequestHandler and output them as JSON
 */

#include "json.h"
#include "serialization.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace transport_catalogue {

class JSONReaderError : public std::runtime_error {    // Ошибка при ошибках парсинга JSONReader
public:
    using runtime_error::runtime_error;
};

class JSONReader {
public:
    explicit JSONReader(std::istream& input, std::ostream& output);

    std::optional<serialization::Serialization_Settings>
    ParseSerializationSettings() const;

    std::optional<renderer::Renderer_Settings>
    ParseRenderSettings() const;

    std::optional<transport_router::TransportRouter::Settings> 
    ParseRouterSettings() const;

    void MakeBase(RequestHandler& request_handler);

    void ProcessStatRequests(renderer::MapRenderer& renderer, 
                             transport_router::TransportRouter& transport_router,
                             RequestHandler& request_handler);
    
private:
    std::istream& input_;
    std::ostream& output_;
    json::Document json_document_;
};

namespace {

void LoadStops(const json::Array& base_requests, RequestHandler& request_handler);
void LoadBusesAndDistances(const json::Array& base_requests, RequestHandler& request_handler);

json::Array ProcessStatRequest(const json::Array& stat_requests, 
                               renderer::MapRenderer& renderer,
                               transport_router::TransportRouter& transport_router,
                               RequestHandler& request_handler);

svg::Color ExtractColor(const json::Node& node);

json::Node 
ProcessStopStatRequest (const json::Node& request, RequestHandler& request_handler);

json::Node 
ProcessBusStatRequest (const json::Node& request, RequestHandler& request_handler);

json::Node 
ProcessMapStatRequest (const json::Node& request, 
                       renderer::MapRenderer& renderer, RequestHandler& request_handler);

json::Node 
ProcessRouteStatRequest (const json::Node& request, 
                         transport_router::TransportRouter& transport_router);

} // namespace

} // namespace transport_catalogue