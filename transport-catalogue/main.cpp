#include <iostream>
#include <fstream>
#include <string_view>
#include <optional>

#include "transport_catalogue.h"
#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"

using namespace std::literals;
using namespace transport_catalogue;
using renderer::MapRenderer;
using transport_router::TransportRouter;
using serialization::Serialization;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

void MakeBase() {
    TransportCatalogue catalogue{};
    RequestHandler request_handler{catalogue};
    
    // build database
    JSONReader json_reader{std::cin, std::cout};
    json_reader.MakeBase(request_handler);

    // build graph
    TransportRouter transport_router{request_handler};
    transport_router.SetSettings(*json_reader.ParseRouterSettings());
    transport_router.Initialize();

    // serialize
    Serialization serialization{*json_reader.ParseSerializationSettings()};
    serialization.SerializeTransportCatalogue(catalogue, transport_router, json_reader.ParseRenderSettings());
}

void ProcessRequests() {
    TransportCatalogue catalogue{};
    JSONReader json_reader{std::cin, std::cout};

    // containers for deserialization
    std::optional<renderer::Renderer_Settings> renderer_settings;
    std::optional<TransportRouter::Settings> router_settings;
    std::unique_ptr<TransportRouter::Graph> ptr_graph = std::make_unique<TransportRouter::Graph>(0);
    std::unique_ptr<TransportRouter::Router> ptr_router = std::make_unique<TransportRouter::Router>(*ptr_graph);
    // deserialize
    Serialization serialization{*json_reader.ParseSerializationSettings()};
    serialization.DeserializeTransportCatalogue(catalogue, 
                                                renderer_settings,
                                                router_settings,
                                                *ptr_graph,
                                                *ptr_router);

    // start map renderer
    MapRenderer renderer{std::cout};
    renderer.SetSettings(std::move(*renderer_settings));

    // start transport router
    RequestHandler request_handler{catalogue};
    TransportRouter transport_router{request_handler};
    transport_router.SetSettings(std::move(*router_settings));
    transport_router.ExternalInitialization(std::move(ptr_graph), std::move(ptr_router));

    // process requests
    json_reader.ProcessStatRequests(renderer, transport_router, request_handler);
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        const std::string_view mode(argv[1]);
        if (mode == "make_base"sv) {
            MakeBase();
            return 0;
        } else if (mode == "process_requests"sv) {
            ProcessRequests();
            return 0;
        }
    }
    
    PrintUsage();
    return 1;
}