#include "transport_router.h"

#include <deque>

#include "graph.h"

namespace transport_catalogue {

namespace transport_router {

using namespace std;
using namespace graph;

TransportRouter::TransportRouter(RequestHandler& request_handler) : 
                                request_handler_(request_handler) {
}

void TransportRouter::SetSettings(Settings&& settings) {
    settings_ = std::move(settings);
    bus_velocity_ = settings_.bus_velocity * KM_H_TO_M_PER_MIN;    // convert km/h to m/min.
}

/* Trace bus route. Gets stops sequence and travel times from previous stop */
vector<pair<graph::VertexId, double>> TransportRouter::TraceBus(const domain::Bus& bus) {
    
    if (bus.stops.empty()) return {};

    /* stop id, travel time from previous stop */
    vector<pair<graph::VertexId, double>> result;
    result.reserve(bus.stops.size());
    
    // push first stop
    graph::VertexId stop_id = bus.stops.front()->id;
    result.push_back({stop_id, 0});

    // go forward (any bus type)
    for (auto it1 = bus.stops.begin(), it2 = it1 + 1; it2 != bus.stops.end(); ++it1, ++it2) {
        double distance = request_handler_.GetDistance((*it1)->id, (*it2)->id);
        if (distance == 0) distance = geo::ComputeDistance((*it1)->coordinates, (*it2)->coordinates);
        double time = distance / bus_velocity_;
        stop_id = (*it2)->id;
        result.push_back({stop_id, time});
    }

    if (bus.type == domain::BusType::LINEAR) {
        // go backward if bus type linear
        for (auto it1 = bus.stops.rbegin(), it2 = it1 + 1; it2 != bus.stops.rend(); ++it1, ++it2) {
            double distance = request_handler_.GetDistance((*it1)->name, (*it2)->name);
            if (distance == 0) distance = geo::ComputeDistance((*it1)->coordinates, (*it2)->coordinates);
            double time = distance / bus_velocity_;
            stop_id = (*it2)->id;
            result.push_back({stop_id, time});
        }
    }
    return result;
}

void TransportRouter::Initialize() {
    // buld the Graph
    size_t vertexes_count = request_handler_.GetAllStops().size();
    ptr_graph_ = make_unique<Graph>(vertexes_count);    // initialize with number of vertexes in graph
    
    for (const auto& bus : request_handler_.GetAllBuses()) {
        vector<pair<graph::VertexId, double>> trace = TraceBus(bus);

        // make edges
        for (auto it1 = trace.begin(); it1 + 1 != trace.end(); ++it1) {
            double total_time = 0;
            int stops_number = 0;
            for (auto it2 = it1 + 1; it2 != trace.end(); ++it2) {
                total_time += it2->second;
                ++stops_number;
                ptr_graph_->AddEdge(
                    graph::Edge<EdgeWeight>{it1->first,
                                            it2->first,
                                            EdgeWeight{total_time + settings_.bus_wait_time,
                                                       stops_number,
                                                       bus.id}});
            }
        }
    }

    // construct Router from Graph
    ptr_router_ = make_unique<Router>(*ptr_graph_);
}

void TransportRouter::Reset() {
    ptr_router_.reset(nullptr);
    ptr_graph_.reset(nullptr);
}

const TransportRouter::InternalState TransportRouter::ExportInternalState() const {
    return TransportRouter::InternalState{settings_, *ptr_graph_, *ptr_router_};
}

void TransportRouter::ExternalInitialization
            (std::unique_ptr<Graph>&& graph, std::unique_ptr<Router>&& router) {
    ptr_graph_ = move(graph);
    ptr_router_ = move(router);
}

Route TransportRouter::GetRoute(const string& from, const string& to) {

    if (!ptr_router_) Initialize();
    
    VertexId vertex_from = request_handler_.FindStop(from)->id;
    VertexId vertex_to = request_handler_.FindStop(to)->id;

    auto info = ptr_router_->BuildRoute(vertex_from, vertex_to);

    if (!info) return {};

    vector<RouteItem> items;
    for (const auto& edge_id : info->edges) {
        const Edge<EdgeWeight>& edge = ptr_graph_->GetEdge(edge_id);
        items.push_back(RouteItem{string_view{request_handler_.FindStop(edge.from)->name},
                                  string_view{request_handler_.FindStop(edge.to)->name},
                                  string_view{request_handler_.FindBus(edge.weight.bus_id)->name},
                                  edge.weight.stops_number,
                                  static_cast<double>(settings_.bus_wait_time),
                                  edge.weight.total_time - settings_.bus_wait_time});
    }
    
    return RouteItems{info->weight.total_time, move(items)};
}

bool operator<(const EdgeWeight& lhs, const EdgeWeight& rhs) {
    return lhs.total_time < rhs.total_time;
}

bool operator>(const EdgeWeight& lhs, const EdgeWeight& rhs) {
    return lhs.total_time > rhs.total_time;
}

EdgeWeight operator+(const EdgeWeight& lhs, const EdgeWeight& rhs) {
    return EdgeWeight{lhs.total_time + rhs.total_time, lhs.stops_number + rhs.stops_number, {}};
}

} // namespace transport_router

} // namespace transport_catalogue