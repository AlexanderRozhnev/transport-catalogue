#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

#include "request_handler.h"
#include "graph.h"
#include "router.h"

namespace transport_catalogue {

namespace transport_router {

constexpr static double KM_H_TO_M_PER_MIN = 1000. / 60.;    /* km/h to m/min. rate */

struct RouteItem {
    std::string_view from_stop;
    std::string_view to_stop;
    std::string_view bus;
    int span_count;
    double wait_time;
    double travel_time;
};

struct RouteItems {
    double total_time;
    std::vector<RouteItem> items;
};

/* Route */
using Route = std::optional<RouteItems>;

/* Route edge, contains weight and route info. */
struct EdgeWeight {
    double total_time;          /* edge time, minutes */
    int stops_number;           /* number of Stops on edge */
    size_t bus_id;              /* bus id */
};

bool operator<(const EdgeWeight& lhs, const EdgeWeight& rhs);
bool operator>(const EdgeWeight& lhs, const EdgeWeight& rhs);
EdgeWeight operator+(const EdgeWeight& lhs, const EdgeWeight& rhs);

class TransportRouter {
public:
    struct Settings {
        double bus_velocity;    /* bus velocity, km/h */
        int bus_wait_time;      /* wait time for bus on stop, minutes */
    };

    using Graph = graph::DirectedWeightedGraph<EdgeWeight>;
    using Router = graph::Router<EdgeWeight>;

    explicit TransportRouter(RequestHandler& request_handler);

    void SetSettings(Settings&& settings);
    Route GetRoute(const std::string& from, const std::string& to);

    void Initialize();
    void Reset();

    struct InternalState {
        const Settings& settings;
        const Graph& graph;
        const Router& router;
    };

    const InternalState ExportInternalState() const;
    void ExternalInitialization(std::unique_ptr<Graph>&& graph, std::unique_ptr<Router>&& router);

private:
    RequestHandler& request_handler_;
    Settings settings_;
    double bus_velocity_;       /* bus velocity, meters/min (converted) */

    std::unique_ptr<Graph> ptr_graph_;
    std::unique_ptr<Router> ptr_router_;

    std::vector<std::pair<graph::VertexId, double>> TraceBus(const domain::Bus& bus);
};

} // namespace transport_router

} // namespace transport_catalogue