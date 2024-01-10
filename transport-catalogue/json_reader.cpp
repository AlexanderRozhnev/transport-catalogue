#include "json_reader.h"

#include <vector>
#include <string>

#include "json_builder.h"

namespace transport_catalogue {

using namespace std;

JSONReader::JSONReader(istream& input, ostream& output) : input_(input),
                                                          output_(output),
                                                          json_document_(json::Load(input)) {
    if (!json_document_.GetRoot().IsDict()) {   /* Elementary check of loaded JSON document */
        throw JSONReaderError("Request file format error."s);
    }
}

/* Parse Serialization settings */
std::optional<serialization::Serialization_Settings>
JSONReader::ParseSerializationSettings() const {
    /*  Process with request: "serialization_settings": { ... } 
    */

    const json::Node& root = json_document_.GetRoot();
    if (root.AsDict().count("serialization_settings") == 0 
        || !root.AsDict().at("serialization_settings").IsDict()) {
    
        throw JSONReaderError("\"serialization_settings\" section format error."s);
    }
    
    serialization::Serialization_Settings serialization_settings;
    const json::Dict& settings = root.AsDict().at("serialization_settings").AsDict();
    // "file": "filename.db"
    serialization_settings.file_name = settings.at("file").AsString();
        
    return serialization_settings;
}

/* Parse render settings */
std::optional<renderer::Renderer_Settings>
JSONReader::ParseRenderSettings() const {
    /* Process with requests: { "render_settings": { ... } }
    */

    const json::Node& root = json_document_.GetRoot();
    if (root.AsDict().count("render_settings") == 0 
        || !root.AsDict().at("render_settings").IsDict()) {
        
        throw JSONReaderError("\"render_settings\" section format error."s);
    }
        
    renderer::Renderer_Settings render_settings;
    const json::Dict& settings = root.AsDict().at("render_settings").AsDict();

    // "width": 1200.0,
    render_settings.width = settings.at("width").AsDouble();
    // "height": 1200.0,
    render_settings.height = settings.at("height").AsDouble();
    // "padding": 50.0,
    render_settings.padding = settings.at("padding").AsDouble();
    // "line_width": 14.0,
    render_settings.line_width = settings.at("line_width").AsDouble();
    // "stop_radius": 5.0,
    render_settings.stop_radius = settings.at("stop_radius").AsDouble();
    // "bus_label_font_size": 20,
    render_settings.bus_label_font_size = settings.at("bus_label_font_size").AsInt();
    // "bus_label_offset": [7.0, 15.0],
    render_settings.bus_label_offset.x = settings.at("bus_label_offset").AsArray()[0].AsDouble();
    render_settings.bus_label_offset.y = settings.at("bus_label_offset").AsArray()[1].AsDouble();
    // "stop_label_font_size": 20,
    render_settings.stop_label_font_size = settings.at("stop_label_font_size").AsInt();
    // "stop_label_offset": [7.0, -3.0],
    render_settings.stop_label_offset.x = settings.at("stop_label_offset").AsArray()[0].AsDouble();
    render_settings.stop_label_offset.y = settings.at("stop_label_offset").AsArray()[1].AsDouble();
    // "underlayer_color": [255, 255, 255, 0.85],
    render_settings.underlayer_color = ExtractColor(settings.at("underlayer_color"));
    // "underlayer_width": 3.0,
    render_settings.underlayer_width = settings.at("underlayer_width").AsDouble();
    // "color_palette": [ "green", [255, 160, 0], "red" ]
    if (settings.at("color_palette").IsArray()) {
        for (const auto& color_node : settings.at("color_palette").AsArray()) {
            render_settings.color_palette.push_back(ExtractColor(color_node));
        }
    }

    return render_settings;
}

/* Parse routning settings */
std::optional<transport_router::TransportRouter::Settings>
JSONReader::ParseRouterSettings() const {
    /* Process with requests: { "routing_settings": { ... } }
    */

    const json::Node& root = json_document_.GetRoot();
    if (root.AsDict().count("routing_settings") == 0 
        || !root.AsDict().at("routing_settings").IsDict()) {
    
        throw JSONReaderError("\"routing_settings\" section format error."s);
    }

    transport_router::TransportRouter::Settings transport_router_settings;
    const json::Dict& settings = root.AsDict().at("routing_settings").AsDict();

    // "bus_velocity": 40,
    transport_router_settings.bus_velocity = settings.at("bus_velocity").AsDouble();
    // "bus_wait_time": 6
    transport_router_settings.bus_wait_time = settings.at("bus_wait_time").AsInt();

    return transport_router_settings;
}

/* Parse Base requests and load them to transport catalogue */
void JSONReader::MakeBase(RequestHandler& request_handler) {
    /* Process with requests: { "base_requests": [ ... ] }
    */
    const json::Node& root = json_document_.GetRoot();
    if (root.AsDict().count("base_requests") == 0 
        || !root.AsDict().at("base_requests").IsArray()) {

        throw JSONReaderError("\"base_requests\" section format error."s);
    }
    
    const json::Array& base_requests = root.AsDict().at("base_requests").AsArray();
    LoadStops(base_requests, request_handler);               // 1st pass
    LoadBusesAndDistances(base_requests, request_handler);   // 2nd pass
}

/* Parse Stat requests, ask transport catalogue and output result to JSON */
void JSONReader::ProcessStatRequests(renderer::MapRenderer &renderer, 
                                 transport_router::TransportRouter &transport_router, 
                                 RequestHandler &request_handler) {
    /* Process with requests: { "stat_requests": [ ... ] }
    */
    const json::Node& root = json_document_.GetRoot();
    if (root.AsDict().count("stat_requests") == 0 
        || !root.AsDict().at("stat_requests").IsArray()) {

        throw JSONReaderError("\"stat_requests\" section format error."s);
    }

    const json::Array& stat_requests = root.AsDict().at("stat_requests").AsArray();
    json::Document result{ProcessStatRequest(stat_requests, renderer, transport_router, request_handler)};
    json::Print(result, output_);
}

namespace {

void LoadStops(const json::Array& base_requests, RequestHandler& request_handler) {
    /* Stop request format: { "type": "Stop", 
                              "name": "Электросети", 
                              "latitude": 43.598701, "longitude": 39.730623,
                              "road_distances": { "Улица Докучаева": 3000, 
                                                  "Улица Лизы Чайкиной": 4300 }
                            }
    */
    for (const auto& request : base_requests) {
        if (request.IsDict() && request.AsDict().count("type") != 0) {
            if (request.AsDict().at("type") != "Stop") continue;
            
            const auto& content = request.AsDict();
            if (content.count("name") != 0 && content.count("latitude") != 0 
                                           && content.count("longitude") != 0) {
                
                request_handler.AddStop(content.at("name").AsString(),
                                  geo::Coordinates{content.at("latitude").AsDouble(), 
                                                   content.at("longitude").AsDouble()});
            } else {
                throw JSONReaderError("Stop request at \"base_requests\" section format error."s);
            }
        } else {
            throw JSONReaderError("\"base_requests\" section format error."s);
        }
    }
}

void LoadBusesAndDistances(const json::Array& base_requests, RequestHandler& request_handler) {
    /* Bus request format: { "type": "Bus",
                             "name": "14",
                             "stops": [ "Улица Лизы Чайкиной",
                                        "Электросети",
                                        "Улица Докучаева",
                                        "Улица Лизы Чайкиной" ],
                             "is_roundtrip": true
                           }
    */
    for (const auto& request : base_requests) {
        /* IsDict() and "type" not checked as it is 2nd pass and it was checked at 1st pass */
        const auto& content = request.AsDict();
        
        if (content.at("type") == "Bus") {
            /* process Bus request */
            if (content.count("name") != 0 && content.count("stops") != 0 
                                           && content.count("is_roundtrip") != 0) {
                vector<string> stops;
                for (const auto& stop_name : content.at("stops").AsArray()) {
                    stops.push_back(stop_name.AsString());
                }
                auto roundtrip = (content.at("is_roundtrip").AsBool()) ? BusType::CIRCULAR : BusType::LINEAR;

                request_handler.AddBus(content.at("name").AsString(), roundtrip, stops);

            } else {
                throw JSONReaderError("Bus request at \"base_requests\" section format error."s);
            }
        } else if (content.at("type") == "Stop" && content.count("road_distances") != 0) {
            /* 2nd pass by Stop - Load real distances */
            /* "road_distances": { "Улица Докучаева": 3000, 
                                   "Улица Лизы Чайкиной": 4300 }
            */
            /* supposed that "road_distances" is not obligate */
            if (content.at("road_distances").IsDict()) {
                if (content.count("name") != 0) {
                    const string& this_stop = content.at("name").AsString();
                    for (const auto& [other_stop, distance] : content.at("road_distances").AsDict()) {
                        request_handler.SetDistance(this_stop, other_stop, distance.AsInt());
                    }
                } else {
                    throw JSONReaderError("Unknown Stop distance request at \"base_requests\" section."s);
                }
            } else {
                throw JSONReaderError("Stop request at \"base_requests\" section format error."s);
            }
        }
    }
}

svg::Color ExtractColor(const json::Node& node) {
    if (node.IsString()) {
        return svg::Color(node.AsString());
    } else if (node.IsArray()) {
        const auto& color = node.AsArray();
        if (color.size() == 3 || color.size() == 4) {
            uint8_t red = static_cast<uint8_t>(color[0].AsInt());
            uint8_t green = static_cast<uint8_t>(color[1].AsInt());
            uint8_t blue = static_cast<uint8_t>(color[2].AsInt());
            if (color.size() == 4) {
                double opacity = color[3].AsDouble();
                return svg::Color{svg::Rgba(red, green, blue, opacity)};
            }
            return svg::Color{svg::Rgb(red, green, blue)};
        }
    }
    return svg::NoneColor;
}

json::Node 
ProcessStopStatRequest (const json::Node& request, RequestHandler& request_handler) {

    const int id = request.AsDict().at("id").AsInt();

    if (request.AsDict().count("name") == 0) {
        throw JSONReaderError("\"stat_requests\" section format error."s);
    }
    const string& name = request.AsDict().at("name").AsString();
    
    auto info = request_handler.GetBusesByStop(name);
    if (info) {
        json::Array buses;
        for (const auto& bus : *info) {
            buses.push_back(json::Node{string(bus)});
        }
        sort(buses.begin(), buses.end(), 
                [](const json::Node& lhs, const json::Node& rhs){
                    return lhs.AsString() < rhs.AsString(); });
        
        return json::Builder{}
                    .StartDict()
                        .Key("request_id"s).Value(id)
                        .Key("buses"s).Value(buses)
                    .EndDict()
                .Build();                       
    }
    
    /* Stop not found */
    return json::Builder{}
                .StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s)
                .EndDict()
            .Build();                       
}

json::Node 
ProcessBusStatRequest (const json::Node& request, RequestHandler& request_handler) {
    const int id = request.AsDict().at("id").AsInt();

    if (request.AsDict().count("name") == 0) {
        throw JSONReaderError("\"stat_requests\" section format error."s);
    }
    const string& name = request.AsDict().at("name").AsString();

    auto info = request_handler.GetBusStat(name);
    if (info) {
        return json::Builder()
                .StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("curvature"s).Value((*info).curvature)
                    .Key("route_length"s).Value((*info).route_length)
                    .Key("stop_count"s).Value((*info).stops_count)
                    .Key("unique_stop_count"s).Value((*info).unique_stops_count)
                .EndDict()
            .Build();
    }
    
    /* Bus not found */
    return json::Builder{}
                .StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("error_message"s).Value("not found"s)
                .EndDict()
            .Build();
}

json::Node 
ProcessMapStatRequest (const json::Node& request, 
                       renderer::MapRenderer& renderer, RequestHandler& request_handler) {
    const int id = request.AsDict().at("id").AsInt();

    /* Map request { "type": "Map", "id": 11111 } */
    std::ostringstream s;
    renderer.RenderMap(request_handler).Render(s);
    return json::Builder{}
                .StartDict()
                    .Key("request_id"s).Value(id)
                    .Key("map"s).Value(s.str())
                .EndDict()
            .Build();
}

json::Node 
ProcessRouteStatRequest (const json::Node& request, 
                         transport_router::TransportRouter& transport_router) {
    const int id = request.AsDict().at("id").AsInt();

    /* Route request {  "type": "Route", 
                        "from": "Biryulyovo Zapadnoye",
                        "to": "Universam",
                        "id": 4 } */
    if (request.AsDict().count("from") == 0 || request.AsDict().count("to") == 0) {
        throw JSONReaderError("\"stat_requests\" section format error."s);
    }
    const string& from = request.AsDict().at("from").AsString();
    const string& to = request.AsDict().at("to").AsString();

    transport_router::Route route = transport_router.GetRoute(from, to);
    
    if (!route) {
        return json::Builder{}
                    .StartDict()
                        .Key("request_id"s).Value(id)
                        .Key("error_message"s).Value("not found")
                    .EndDict()
                .Build();
    }
    
    json::Array json_route_items;
    for (const transport_router::RouteItem& item : route.value().items) {
        json::Node stop = json::Builder()
                                .StartDict()
                                    .Key("type"s).Value("Wait"s)
                                    .Key("stop_name"s).Value(string{item.from_stop})
                                    .Key("time"s).Value(item.wait_time)
                                .EndDict()
                            .Build();
        json_route_items.push_back(move(stop));
        json::Node bus =  json::Builder()
                                .StartDict()
                                    .Key("type"s).Value("Bus"s)
                                    .Key("bus"s).Value(string{item.bus})
                                    .Key("span_count"s).Value(item.span_count)
                                    .Key("time"s).Value(item.travel_time)
                                .EndDict()
                            .Build();
        json_route_items.push_back(move(bus));
    }

    return json::Builder{}
                .StartDict()
                    .Key("items"s).Value(json_route_items)
                    .Key("request_id"s).Value(id)
                    .Key("total_time"s).Value(route->total_time)
                .EndDict()
            .Build();
}

json::Array ProcessStatRequest(const json::Array& stat_requests, 
                        renderer::MapRenderer& renderer,
                        transport_router::TransportRouter& transport_router,
                        RequestHandler& request_handler) {
    json::Array result{};

    for (const auto& request : stat_requests) {
        if (request.IsDict() && request.AsDict().count("type") != 0
                            && request.AsDict().count("id") != 0) {
                       
            const string& type = request.AsDict().at("type").AsString();

            if (type == "Stop"s) {
                result.push_back(ProcessStopStatRequest(request, request_handler));
            } else if (type == "Bus"s) {
                result.push_back(ProcessBusStatRequest(request, request_handler));
            } else if (type == "Map"s) {
                result.push_back(ProcessMapStatRequest(request, renderer, request_handler));
            } else if (type == "Route") {
                result.push_back(ProcessRouteStatRequest(request, transport_router));
            }
        } else {
            throw JSONReaderError("\"stat_requests\" section format error."s);   
        }
    }
    return result;
}

} // namespace

} // namespace transport_catalogue