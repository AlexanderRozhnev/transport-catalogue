#include "serialization.h"

#include <fstream>
#include <deque>

namespace transport_catalogue {

namespace serialization {

using namespace std;

void Serialization::SetSettings(Serialization_Settings&& settings) {
    settings_ = std::move(settings);
}

void Serialization::ClearContainers() {
    serialized_catalogue_.Clear();
}

/* Serialize Stops */
tc_proto::GeoCoordinates MakeProtoGeoCoordinates(double latitude, double longitude) {
    tc_proto::GeoCoordinates p_coordinates;
    p_coordinates.set_latitude(latitude);
    p_coordinates.set_longitude(longitude);
    return p_coordinates;
}

tc_proto::Stop MakeProtoStop(const domain::Stop& stop) {
    tc_proto::Stop p_stop;
    p_stop.set_id(stop.id);
    p_stop.set_name(stop.name);
    *p_stop.mutable_coordinates() = 
                MakeProtoGeoCoordinates(stop.coordinates.lat, stop.coordinates.lng);
    return p_stop;
}

void Serialization::SaveStops(const TransportCatalogue& transport_catalogue) {
    const auto stops = transport_catalogue.GetAllStops();
    for (const auto& stop : stops) {
        tc_proto::Stop* new_stop = serialized_catalogue_.add_stops();
        *new_stop = MakeProtoStop(stop);
    }
}

/* Serialize Buses */

tc_proto::BusType MakeProtoBusType(const domain::BusType bus_type) {
    tc_proto::BusType p_bus_type;
    switch (bus_type) {
        case domain::BusType::LINEAR :
            p_bus_type = tc_proto::BusType::LINEAR;
            break;
        case domain::BusType::CIRCULAR :
            p_bus_type = tc_proto::BusType::CIRCULAR;
            break;
        default :
            p_bus_type = tc_proto::BusType::UNKNOWN;
    }
    return p_bus_type;
}

tc_proto::Bus MakeProtoBus(const domain::Bus& bus) {
    tc_proto::Bus p_bus;
    p_bus.set_id(bus.id);
    p_bus.set_bus_type(MakeProtoBusType(bus.type));
    p_bus.set_bus_name(bus.name);
    for (const domain::Stop* stop : bus.stops) {
        p_bus.add_stop_ids(stop->id);
    }
    return p_bus;
}

void Serialization::SaveBuses(const TransportCatalogue& transport_catalogue) {
    auto buses = transport_catalogue.GetAllBuses();
    for (const auto& bus : buses) {
        *serialized_catalogue_.add_buses() = MakeProtoBus(bus);
    }
}

/* Serialize Distances */

tc_proto::Distance 
MakeProtoDistances(uint32_t stop_id_from, uint32_t stop_id_to, int distance) {
    tc_proto::Distance p_distance;
    p_distance.set_stop_id_from(stop_id_from);
    p_distance.set_stop_id_to(stop_id_to);
    p_distance.set_distance(distance);
    return p_distance;
}

void Serialization::SaveDistances(const TransportCatalogue& transport_catalogue) {
    const auto& distances = transport_catalogue.GetDistances();
    for (const auto& [stops, distance] : distances) {
        size_t stop_id_from = stops.first->id;
        size_t stop_id_to = stops.second->id;
        *serialized_catalogue_.add_distances() = 
                    MakeProtoDistances(stop_id_from, stop_id_to, distance);
    }
}

/* Serialize Renderer Settings */

svg_proto::Point MakeProtoPoint(const svg::Point& point) {
    svg_proto::Point p_point;
    p_point.set_x(point.x);
    p_point.set_y(point.y);
    return p_point;
}

svg_proto::Color MakeProtoColor(const svg::Color& color) {
    svg_proto::Color p_color;
    if (holds_alternative<string>(color)) {
        *p_color.mutable_color_string() = get<string>(color);
    } else if (holds_alternative<svg::Rgb>(color)) {
        const svg::Rgb& c = get<svg::Rgb>(color);
        auto p = p_color.mutable_color_rgb();
        p->set_r(c.red);
        p->set_g(c.green);
        p->set_b(c.blue);
    } else if (holds_alternative<svg::Rgba>(color)) {
        const svg::Rgba& c = get<svg::Rgba>(color);
        auto p = p_color.mutable_color_rgba();
        p->set_r(c.red);
        p->set_g(c.green);
        p->set_b(c.blue);
        p->set_o(c.opacity);
    }
    return p_color;
}

void Serialization::SaveRendererSettings
                    (const std::optional<renderer::Renderer_Settings>& renderer_settings) {
    if (!renderer_settings) return;

    mr_proto::MapRenderSettings* p_settings = 
                    serialized_catalogue_.mutable_map_renderer_settings();

    p_settings->set_width(renderer_settings->width);
    p_settings->set_height(renderer_settings->height);
    p_settings->set_padding(renderer_settings->padding);
    p_settings->set_line_width(renderer_settings->line_width);
    p_settings->set_stop_radius(renderer_settings->stop_radius);
    p_settings->set_bus_label_font_size(renderer_settings->bus_label_font_size);
    *p_settings->mutable_bus_label_offset() = 
                    MakeProtoPoint(renderer_settings->bus_label_offset);

    p_settings->set_stop_label_font_size(renderer_settings->stop_label_font_size);
    *p_settings->mutable_stop_label_offset() = 
                    MakeProtoPoint(renderer_settings->stop_label_offset);

    *p_settings->mutable_underlayer_color() = 
                    MakeProtoColor(renderer_settings->underlayer_color);

    p_settings->set_underlayer_width(renderer_settings->underlayer_width);
    for (const auto& color : renderer_settings->color_palette) {
        *p_settings->add_color_palette() = MakeProtoColor(color);
    }
}

/* Serialize Transport Router settings */

void Serialization::SaveTransportRouterSettings(const TransportRouter::Settings& router_settings) {
   
    tr_proto::RouterSettings* p_settings = 
                    serialized_catalogue_.mutable_router_settings();
    p_settings->set_bus_velocity(router_settings.bus_velocity);
    p_settings->set_bus_wait_time(router_settings.bus_wait_time);
}

/* Serialize Graph */

g_proto::EdgeWeight MakeProtoEdgeWeight(const graph::Edge<transport_router::EdgeWeight>& edge) {
    g_proto::EdgeWeight p_edge_weight;
    p_edge_weight.set_total_time(edge.weight.total_time);
    p_edge_weight.set_stops_number(edge.weight.stops_number);
    p_edge_weight.set_bus_id(edge.weight.bus_id);
    return p_edge_weight;
}

g_proto::Edge MakeProtoEdge(const graph::Edge<transport_router::EdgeWeight>& edge) {
    g_proto::Edge p_edge;
    p_edge.set_vertex_id_from(edge.from);
    p_edge.set_vertex_id_to(edge.to);
    *p_edge.mutable_weight() = MakeProtoEdgeWeight(edge);
    return p_edge;
}

g_proto::IncidenceList MakeProtoIncedenceList
            (const ranges::Range<std::vector<graph::EdgeId>::const_iterator> incident_edges) {
    
    g_proto::IncidenceList p_incident_list{};
    for (const auto& edge : incident_edges) {
        p_incident_list.add_edge_id(edge);
    }
    return p_incident_list;
}

void Serialization::SaveGraph(const std::optional<TransportRouter::Graph>& graph) {
    if (!graph) return;

    g_proto::Graph* p_graph = serialized_catalogue_.mutable_graph();

    // fill edges
    const size_t edges_count = graph->GetEdgeCount();
    for (graph::VertexId edge_id = 0; edge_id < edges_count; ++edge_id) {
        *p_graph->add_edges() = MakeProtoEdge(graph->GetEdge(edge_id));
    }

    // fill IncidenceList - Maybe NOT NEEDED !!!
    const size_t vertex_count = graph->GetVertexCount();
    for (graph::VertexId vertex = 0; vertex < vertex_count; ++vertex) {
        *p_graph->add_incedence_lists() = 
                    MakeProtoIncedenceList(graph->GetIncidentEdges(vertex));
    }
}

/* Serialize Router */

tr_proto::Weight MakeProtoWeight(const transport_router::EdgeWeight& weight) {
    tr_proto::Weight p_weight;
    p_weight.set_total_time(weight.total_time);
    p_weight.set_stops_number(weight.stops_number);
    p_weight.set_bus_id(weight.bus_id);
    return p_weight;
}

tr_proto::RouteData MakeProtoRouteData
            (const optional<TransportRouter::Router::RouteInternalData>& opt_route_data) {
    tr_proto::RouteData p_route_data;
    *p_route_data.mutable_weight() = MakeProtoWeight(opt_route_data->weight);
    if (opt_route_data->prev_edge.has_value()) {
        p_route_data.set_prev_edge(*(opt_route_data->prev_edge));
    }
    return p_route_data;
}

void Serialization::SaveRouter(const TransportRouter::Router& router) {
    tr_proto::RoutesInternalData* 
    p_routes_internal_data = serialized_catalogue_.mutable_routes_internal_data();

    graph::RouterInternalState internal_data = router.ExportInternalState();
    for (const auto& vector_of_data : internal_data.routes_internal_data) {
        auto new_vector = p_routes_internal_data->add_routes_internal_data();
        for (const auto& opt_route_data : vector_of_data) {
            auto new_vector_opt_route_data = new_vector->add_optional_route_data();
            if (opt_route_data.has_value()) {
                *new_vector_opt_route_data->mutable_route_data() = 
                                            MakeProtoRouteData(opt_route_data);
            }
        }
    }
}

bool Serialization::SerializeTransportCatalogue
                (const TransportCatalogue& transport_catalogue,
                 TransportRouter& transport_router, 
                 const optional<renderer::Renderer_Settings>& renderer_settings) {
    ofstream out(settings_.file_name, ios::binary);
    if (!out) {
        return false;   // error opening out file
    }
   
    SaveStops(transport_catalogue);
    SaveBuses(transport_catalogue);
    SaveDistances(transport_catalogue);

    if (renderer_settings) {
        SaveRendererSettings(*renderer_settings);
    }

    SaveTransportRouterSettings(transport_router.ExportInternalState().settings);
    
    SaveGraph(transport_router.ExportInternalState().graph);
    SaveRouter(transport_router.ExportInternalState().router);

    bool result = serialized_catalogue_.SerializeToOstream(&out);
    ClearContainers();
    return result;
}

/* --- Deserialization --- */

/* Deserialize Stops */

void Serialization::LoadStops(TransportCatalogue& transport_catalogue) {
    for (const auto& p_stop : serialized_catalogue_.stops()) {
        transport_catalogue.AddStop(p_stop.id(),
                                    p_stop.name(),
                                    geo::Coordinates{p_stop.coordinates().latitude(),
                                                     p_stop.coordinates().longitude()});
    }
}

/* Deserialize Buses */

domain::BusType MakeDomainBusType(const tc_proto::BusType p_bus_type) {
    domain::BusType bus_type;
    switch (p_bus_type) {
        case tc_proto::BusType::LINEAR:
            bus_type = domain::BusType::LINEAR;
            break;
        case tc_proto::BusType::CIRCULAR :
            bus_type = domain::BusType::CIRCULAR;
            break;
        default :
            bus_type = domain::BusType::UNKNOWN;
    }
    return bus_type;
}

void Serialization::LoadBuses(TransportCatalogue& transport_catalogue) {
    for (const auto& p_bus : serialized_catalogue_.buses()) {
        vector<string> stop_names;
        stop_names.reserve(p_bus.stop_ids_size());
        for (auto stop_id : p_bus.stop_ids()) {
            stop_names.emplace_back(transport_catalogue.FindStop(stop_id)->name);
        }
        transport_catalogue.AddBus(p_bus.id(),
                                   p_bus.bus_name(),
                                   MakeDomainBusType(p_bus.bus_type()),
                                   move(stop_names));
    }
}

/* Deserialize Distances */

void Serialization::LoadDistances(TransportCatalogue& transport_catalogue) {
    for (const auto& p_distance : serialized_catalogue_.distances()) {
        size_t stop_id_from = p_distance.stop_id_from();
        size_t stop_id_to = p_distance.stop_id_to();
        const uint32_t distance = p_distance.distance();
        transport_catalogue.SetDistance(stop_id_from, stop_id_to, distance);
    }
}

/* Desrialize Renderer Settings */

svg::Point MakePoint(svg_proto::Point p_point) {
    return svg::Point{p_point.x(), p_point.y()};
}

svg::Color MakeColor(const svg_proto::Color& p_color) {
    svg::Color color;
    switch (p_color.color_case()) {
        case svg_proto::Color::kColorString :
            color = p_color.color_string();
            break;
        case svg_proto::Color::kColorRgb :
            color = svg::Rgb{static_cast<uint8_t>(p_color.color_rgb().r()), 
                             static_cast<uint8_t>(p_color.color_rgb().g()),
                             static_cast<uint8_t>(p_color.color_rgb().b()) };
            break;
        case svg_proto::Color::kColorRgba :
            color = svg::Rgba{static_cast<uint8_t>(p_color.color_rgba().r()),
                              static_cast<uint8_t>(p_color.color_rgba().g()),
                              static_cast<uint8_t>(p_color.color_rgba().b()),
                              p_color.color_rgba().o() };
            break;
    }
    return color;
}

void Serialization::LoadRendererSettings(optional<renderer::Renderer_Settings>& renderer_settings) {
    if (!serialized_catalogue_.has_map_renderer_settings()) return;

    const auto& p_settings = serialized_catalogue_.map_renderer_settings();
    renderer::Renderer_Settings temp;

    temp.width = p_settings.width();
    temp.height = p_settings.height();
    temp.padding = p_settings.padding();
    temp.line_width = p_settings.line_width();
    temp.stop_radius = p_settings.stop_radius();
    temp.bus_label_font_size = p_settings.bus_label_font_size();
    temp.bus_label_offset = MakePoint(p_settings.bus_label_offset());
    temp.stop_label_font_size = p_settings.stop_label_font_size();
    temp.stop_label_offset = MakePoint(p_settings.stop_label_offset());
    temp.underlayer_color = MakeColor(p_settings.underlayer_color());
    temp.underlayer_width = p_settings.underlayer_width();
    for (const auto& p_color : p_settings.color_palette()) {
        temp.color_palette.emplace_back(MakeColor(p_color));
    }

    renderer_settings.emplace(move(temp));
}

/* Deserialize Router Settings */

optional<TransportRouter::Settings> Serialization::LoadRouterSettings() {
    return transport_router::TransportRouter::Settings{
                serialized_catalogue_.router_settings().bus_velocity(),
                serialized_catalogue_.router_settings().bus_wait_time()};
}

/* Deserialize Graph */

transport_router::EdgeWeight MakeEdgeWeight(const g_proto::EdgeWeight& p_weight) {
    transport_router::EdgeWeight edge_weight;
    edge_weight.total_time = p_weight.total_time();
    edge_weight.stops_number = p_weight.stops_number();
    edge_weight.bus_id = p_weight.bus_id();
    return edge_weight;
}

graph::Edge<transport_router::EdgeWeight> MakeEdge(const g_proto::Edge& p_edge) {
    graph::Edge<transport_router::EdgeWeight> edge;
    edge.from = p_edge.vertex_id_from();
    edge.to = p_edge.vertex_id_to();
    edge.weight = MakeEdgeWeight(p_edge.weight());
    return edge;
}

vector<graph::Edge<transport_router::EdgeWeight>> LoadGraphEdges(const g_proto::Graph& p_graph) {
    vector<graph::Edge<transport_router::EdgeWeight>> edges;
    for (const auto& p_edge : p_graph.edges()) {
        edges.push_back(MakeEdge(p_edge));
    }
    return edges;
}

vector<graph::EdgeId> MakeIncedenceList(const g_proto::IncidenceList& p_incedence_list) {
    vector<graph::EdgeId> incedence_list;
    incedence_list.reserve(p_incedence_list.edge_id_size());
    for (const auto& edge_id : p_incedence_list.edge_id()) {
        incedence_list.push_back(edge_id);
    }
    return incedence_list;
}

vector<vector<graph::EdgeId>> LoadGraphIncedenceList(const g_proto::Graph& p_graph) {
    vector<vector<graph::EdgeId>> incidence_list;
    for (const auto& p_incidence_list : p_graph.incedence_lists()) {
        incidence_list.push_back(MakeIncedenceList(p_incidence_list));
    }
    return incidence_list;
}

 void Serialization::LoadGraph(TransportRouter::Graph& graph) {
    if (!serialized_catalogue_.has_graph()) return;

    const auto& p_graph = serialized_catalogue_.graph();
    graph.ExternalInitialization(LoadGraphEdges(p_graph), 
                                 LoadGraphIncedenceList(p_graph));
}

/* Deserialize Router */

transport_router::EdgeWeight MakeWeight(const tr_proto::Weight& p_weight) {
    transport_router::EdgeWeight edge_weight;
    edge_weight.total_time = p_weight.total_time();
    edge_weight.stops_number = p_weight.stops_number();
    edge_weight.bus_id = p_weight.bus_id();
    return edge_weight;
}

optional<TransportRouter::Router::RouteInternalData>
MakeRouteInternalData(const tr_proto::OptionalRouteData& p_route_data) {
    TransportRouter::Router::RouteInternalData internal_data;
    if (!p_route_data.has_route_data()) {
        return nullopt;
    }
    internal_data.weight = MakeWeight(p_route_data.route_data().weight());
    optional<graph::EdgeId> prev_edge;
    if (p_route_data.route_data().optional_prev_edge_case() == 
            tr_proto::RouteData::OptionalPrevEdgeCase::kPrevEdge) {
        internal_data.prev_edge = p_route_data.route_data().prev_edge();
    } else {
        internal_data.prev_edge = nullopt;
    }
    return internal_data;
}

TransportRouter::Router::RoutesInternalData Serialization::LoadRouterData() {
    TransportRouter::Router::RoutesInternalData routes_internal_data;

    const tr_proto::RoutesInternalData& 
    p_routes_internal_data = serialized_catalogue_.routes_internal_data();

    for (const auto& p_vector : p_routes_internal_data.routes_internal_data()) {
        vector<optional<TransportRouter::Router::RouteInternalData>> vector_data;
        for (const auto& p_route_data : p_vector.optional_route_data()) {
            vector_data.emplace_back(MakeRouteInternalData(p_route_data));
        }
        routes_internal_data.emplace_back(vector_data);
    }
    return routes_internal_data;
}

bool Serialization::DeserializeTransportCatalogue
                    (TransportCatalogue& transport_catalogue,
                     std::optional<renderer::Renderer_Settings>& renderer_settings,
                     std::optional<TransportRouter::Settings>& transport_router_settings,
                     TransportRouter::Graph& graph,
                     TransportRouter::Router& router) {

    ifstream in(settings_.file_name, ios::binary);
    if (!in) {
        return false;       // Error open file
    }    

    if (!serialized_catalogue_.ParseFromIstream(&in)) {
        return false;       // Error getting serialized data
    }

    LoadStops(transport_catalogue);
    LoadBuses(transport_catalogue);
    LoadDistances(transport_catalogue);

    if (serialized_catalogue_.has_map_renderer_settings()) {
        LoadRendererSettings(renderer_settings);
    }

    if (serialized_catalogue_.has_router_settings()) {
        transport_router_settings = *LoadRouterSettings();
    } else {
        transport_router_settings = nullopt;
    }

    LoadGraph(graph);
    router.ExternalInitialization(LoadRouterData());

    ClearContainers();
    return true;
}

} // namespace serialization

} // namespace transport_catalogue