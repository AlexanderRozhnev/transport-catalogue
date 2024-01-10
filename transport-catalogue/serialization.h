#pragma once

/* Serializes and Deserializes transport catalogue
*/
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>
#include <transport_router.pb.h>
#include <graph.pb.h>

namespace transport_catalogue {
namespace serialization {

using transport_router::TransportRouter;

struct Serialization_Settings {
    std::string file_name;
};

class Serialization {
public:
    explicit Serialization(Serialization_Settings&& settings) : settings_(std::move(settings)) {}
    
    void SetSettings(Serialization_Settings&& settings);

    bool SerializeTransportCatalogue(const TransportCatalogue& transport_catalogue,
                                     TransportRouter& transport_router,
                                     const std::optional<renderer::Renderer_Settings>& renderer_settings);

    bool DeserializeTransportCatalogue(TransportCatalogue& transport_catalogue,
                                       std::optional<renderer::Renderer_Settings>& renderer_settings,
                                       std::optional<TransportRouter::Settings>& transport_router_settings,
                                       TransportRouter::Graph& graph,
                                       TransportRouter::Router& router);
private:
    Serialization_Settings settings_;
    tc_proto::TransportCatalogue serialized_catalogue_;
    void ClearContainers();

    /* Serialization */
    void SaveStops(const TransportCatalogue& transport_catalogue);
    void SaveBuses(const TransportCatalogue& transport_catalogue);
    void SaveDistances(const TransportCatalogue& transport_catalogue);
    void SaveRendererSettings(const std::optional<renderer::Renderer_Settings>& renderer_settings);
    void SaveTransportRouterSettings(const TransportRouter::Settings& router_settings);
    void SaveGraph(const std::optional<TransportRouter::Graph>& graph);
    void SaveRouter(const TransportRouter::Router& router);

    /* Deserialization */
    void LoadStops(TransportCatalogue& transport_catalogue);
    void LoadBuses(TransportCatalogue& transport_catalogue);
    void LoadDistances(TransportCatalogue& transport_catalogue);
    void LoadRendererSettings(std::optional<renderer::Renderer_Settings>& renderer_settings);
    std::optional<TransportRouter::Settings> LoadRouterSettings();
    void LoadGraph(TransportRouter::Graph& graph);
    TransportRouter::Router::RoutesInternalData LoadRouterData();
};

/* Serialize Stops */
tc_proto::GeoCoordinates MakeProtoGeoCoordinates(double latitude, double longitude);
tc_proto::Stop MakeProtoStop(const domain::Stop& stop);

/* Serialize Buses */
tc_proto::BusType MakeProtoBusType(const domain::BusType bus_type);
tc_proto::Bus MakeProtoBus(const domain::Bus& bus);

/* Serialize Distances */
tc_proto::Distance MakeProtoDistances
            (uint32_t stop_id_from, uint32_t stop_id_to, int distance);

/* Serialize Renderer Settings */
svg_proto::Point MakeProtoPoint(const svg::Point& point);
svg_proto::Color MakeProtoColor(const svg::Color& color);

/* Serialize Graph */
g_proto::EdgeWeight MakeProtoEdgeWeight(const graph::Edge<transport_router::EdgeWeight>& edge);
g_proto::Edge MakeProtoEdge(const graph::Edge<transport_router::EdgeWeight>& edge);
g_proto::IncidenceList MakeProtoIncedenceList
            (const ranges::Range<std::vector<graph::EdgeId>::const_iterator> incident_edges);

/* Serialize Router */
tr_proto::Weight MakeProtoWeight(const transport_router::EdgeWeight& weight);
tr_proto::RouteData MakeProtoRouteData
            (const std::optional<TransportRouter::Router::RouteInternalData>& route_data);

/* Deserialize Buses */
domain::BusType MakeDomainBusType(const tc_proto::BusType p_bus_type);

/* Desrialize Renderer Settings */
svg::Point MakePoint(svg_proto::Point p_point);
svg::Color MakeColor(const svg_proto::Color& p_color);

/* Deserialize Graph */
transport_router::EdgeWeight MakeEdgeWeight(const g_proto::EdgeWeight& p_weight);
graph::Edge<transport_router::EdgeWeight> MakeEdge(const g_proto::Edge& p_edge);
std::vector<graph::EdgeId> MakeIncedenceList(const g_proto::IncidenceList& p_incedence_list);

/* Deserialize Router */
transport_router::EdgeWeight MakeWeight(const tr_proto::Weight& p_weight);
std::optional<TransportRouter::Router::RouteInternalData> MakeRouteInternalData
            (const tr_proto::OptionalRouteData& p_route_data);

} // namespace serialization

} // namespace transport_catalogue