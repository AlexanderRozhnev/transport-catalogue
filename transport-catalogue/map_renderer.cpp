#include "map_renderer.h"

#include <vector>
#include <limits>

using namespace std;

namespace transport_catalogue {

namespace renderer {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

SphereProjector::SphereProjector(double min_lon, double max_lon, double min_lat, double max_lat, 
                        double max_width, double max_height, double padding) : padding_(padding),
                                                                               min_lon_(min_lon),
                                                                               max_lat_(max_lat) {
    // Вычисляем коэффициент масштабирования вдоль координаты x
    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }
        
    // Вычисляем коэффициент масштабирования вдоль координаты y
    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        // Коэффициенты масштабирования по ширине и высоте ненулевые,
        // берём минимальный из них
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        // Коэффициент масштабирования по ширине ненулевой, используем его
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        // Коэффициент масштабирования по высоте ненулевой, используем его
        zoom_coeff_ = *height_zoom;
    }
}

Lines::Lines(const SphereProjector& projector, 
            const domain::BusType bus_type, 
            const vector<const domain::Stop*>& stops, 
            const svg::Color color, const Renderer_Settings& render_settings) : bus_type_(bus_type),
                                                                              color_(color) {
    stroke_width_ = render_settings.line_width;
    for (const auto& stop : stops) {
        points_.push_back(projector(stop->coordinates));
    }
}

void Lines::Draw(svg::ObjectContainer &container) const {
    
    svg::Polyline route_line;
    route_line.SetStrokeColor(color_)
              .SetFillColor(svg::NoneColor)
              .SetStrokeWidth(stroke_width_)
              .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
              .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    for (const auto& stop_point : points_) {
        route_line.AddPoint(stop_point);
    }
    // linear bus route type
    if ((bus_type_ == domain::BusType::LINEAR) && (points_.size() > 1)) {
        for (auto it = points_.rbegin() + 1; it != points_.rend(); ++it) {
            route_line.AddPoint(*it);
        }
    }
    container.Add(route_line);
}

BusNames::BusNames(const SphereProjector& projector,
                   const domain::BusType bus_type, const string& bus_name, 
                   const std::vector<const domain::Stop*>& stops, 
                   const svg::Color color, const Renderer_Settings& render_settings) : bus_type_(bus_type),
                                                                                       color_(color),
                                                                                       bus_name_(bus_name) {
    if (!stops.empty()) {
        coordinates_.push_back(projector(stops.front()->coordinates));
        if ((bus_type_ == domain::BusType::LINEAR) && (stops.front()->name != stops.back()->name)) {
            coordinates_.push_back(projector(stops.back()->coordinates));
        }
    }
    bus_label_offset_ = render_settings.bus_label_offset;
    font_size_ = static_cast<uint32_t>(render_settings.bus_label_font_size);
    underlayer_color_ = render_settings.underlayer_color;
    underlayer_width_ = render_settings.underlayer_width;
}

void BusNames::DrawName(svg::ObjectContainer& container, 
                                const svg::Point& pos, const string& text) const {
    svg::Text bus_name;
    bus_name.SetPosition(pos)
             .SetOffset(bus_label_offset_)
             .SetFontSize(font_size_)
             .SetFontFamily("Verdana")
             .SetFontWeight("bold")
             .SetData(text);
    svg::Text bus_name_underlayer{bus_name};
    bus_name.SetFillColor(color_);
    bus_name_underlayer.SetFillColor(underlayer_color_)
                        .SetStrokeColor(underlayer_color_)
                        .SetStrokeWidth(underlayer_width_)
                        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    container.Add(bus_name_underlayer);
    container.Add(bus_name);
}

void BusNames::Draw(svg::ObjectContainer& container) const {
    // draw Bus name at all stops
    for (const auto& pos : coordinates_) {
        DrawName(container, pos, bus_name_);
    }
}

StopPoint::StopPoint(const SphereProjector &projector, const domain::Stop* stop, 
                                    const Renderer_Settings& render_settings) {
    coordinates_ = projector(stop->coordinates);
    radius_ = render_settings.stop_radius;
}

void StopPoint::Draw(svg::ObjectContainer &container) const {
    container.Add(svg::Circle{}.SetCenter(coordinates_).SetRadius(radius_).SetFillColor("white"));
}

StopName::StopName(const SphereProjector &projector, const domain::Stop *stop, 
                                    const Renderer_Settings &render_settings) {
    coordinates_ = projector(stop->coordinates);
    stop_name_ = stop->name;
    stop_label_offset_ = render_settings.stop_label_offset;
    stop_label_font_size_ = static_cast<uint32_t>(render_settings.stop_label_font_size);
    underlayer_color_ = render_settings.underlayer_color;
    underlayer_width_ = render_settings.underlayer_width;
}

void StopName::DrawName(svg::ObjectContainer& container, 
                                const svg::Point& pos, const string& text) const {
    svg::Text bus_name;
    bus_name.SetPosition(pos)
             .SetOffset(stop_label_offset_)
             .SetFontSize(stop_label_font_size_)
             .SetFontFamily("Verdana")
             .SetData(text);
    svg::Text bus_name_underlayer{bus_name};
    bus_name.SetFillColor("black");
    bus_name_underlayer.SetFillColor(underlayer_color_)
                        .SetStrokeColor(underlayer_color_)
                        .SetStrokeWidth(underlayer_width_)
                        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    container.Add(bus_name_underlayer);
    container.Add(bus_name);
}

void StopName::Draw(svg::ObjectContainer &container) const {
    DrawName(container, coordinates_, stop_name_);
}

template <typename DrawableIterator>
void DrawPicture(DrawableIterator begin, DrawableIterator end, svg::ObjectContainer& target) {
    for (auto it = begin; it != end; ++it) {
        (*it)->Draw(target);
    }
}

template <typename Container>
void DrawPicture(const Container& container, svg::ObjectContainer& target) {
    DrawPicture(begin(container), end(container), target);
}

MapRenderer::MapRenderer(std::ostream &output) : output_(output) {
}

svg::Document MapRenderer::RenderMap(RequestHandler &request_handler) const {

    // get info about routes and translate them to map render
    const auto& buses = request_handler.GetAllBuses();

    // get all Bus names and min/max longitude/latitude
    vector<string_view> bus_names;
    double min_lon = std::numeric_limits<double>::max();
    double max_lon = std::numeric_limits<double>::lowest();
    double min_lat = std::numeric_limits<double>::max();
    double max_lat = std::numeric_limits<double>::lowest();
    for (const auto& bus : buses) {
        bus_names.push_back(string_view(bus.name));
        for (const auto stop : bus.stops) {
            if (stop->coordinates.lng < min_lon) min_lon = stop->coordinates.lng;
            if (stop->coordinates.lng > max_lon) max_lon = stop->coordinates.lng;
            if (stop->coordinates.lat < min_lat) min_lat = stop->coordinates.lat;
            if (stop->coordinates.lat > max_lat) max_lat = stop->coordinates.lat;
        }
    }
    
    // sort bus-names
    sort(bus_names.begin(), bus_names.end());

    // initialize SphereProjector
    const SphereProjector projector{min_lon, max_lon, min_lat, max_lat, 
            render_settings_.width, render_settings_.height, render_settings_.padding};

    // Compose picture
    vector<unique_ptr<svg::Drawable>> lines_layer;
    vector<unique_ptr<svg::Drawable>> bus_names_layer;
    vector<unique_ptr<svg::Drawable>> stop_points_layer;
    vector<unique_ptr<svg::Drawable>> stop_names_layer;

    size_t color_counter = 0;
    for (const auto& name : bus_names) {
        const domain::Bus* bus = request_handler.FindBus(name);
        
        if (!bus->stops.empty()) {
            // select color from palette
            svg::Color route_color = render_settings_.color_palette[color_counter];
            color_counter = ((color_counter + 1) < render_settings_.color_palette.size()) ? color_counter + 1 : 0;
            
            lines_layer.emplace_back(make_unique<Lines>(projector, bus->type, 
                                                                bus->stops, route_color, render_settings_));
            bus_names_layer.emplace_back(make_unique<BusNames>(projector, bus->type, string{name}, 
                                                                bus->stops, route_color, render_settings_));
        }
    }

    // Get stop names and sort
    const auto& stops = request_handler.GetAllStops();
    vector<string_view> stop_names;
    for (const auto& stop : stops) {
        // if no buses pass the stop, stop is not needed
        if (request_handler.GetBusesByStop(stop.name).has_value()
            && !request_handler.GetBusesByStop(stop.name).value().empty()) {
            stop_names.push_back(string_view(stop.name));
        }
    }
    sort(stop_names.begin(), stop_names.end());

    // Compose Stop layers
    for (const auto& name : stop_names) {
        const domain::Stop* stop = request_handler.FindStop(name);
        stop_points_layer.emplace_back(make_unique<StopPoint>(projector, stop, render_settings_));
        stop_names_layer.emplace_back(make_unique<StopName>(projector, stop, render_settings_));
    }

    // Draw picture
    svg::Document result{};
    DrawPicture(lines_layer, result);
    DrawPicture(bus_names_layer, result);
    DrawPicture(stop_points_layer, result);
    DrawPicture(stop_names_layer, result);

    return result;
}

} // namespace renderer

} // namespace transport_catalogue