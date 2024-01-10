#pragma once

/*
 * Код, отвечающий за визуализацию карты маршрутов в формате SVG.
 */

#include "geo.h"
#include "svg.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

#include "request_handler.h"

namespace transport_catalogue {

namespace renderer {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    SphereProjector(double min_lon, double max_lon, double min_lat, double max_lat,
                    double max_width, double max_height, double padding);

    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding);

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                 (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

/* Renderer settings */
struct Renderer_Settings {
    double width;               // ширина изображения в пикселях. Вещественное число в диапазоне от 0 до 100000.
    double height;              // высота изображения в пикселях. Вещественное число в диапазоне от 0 до 100000.
    double padding;             // отступ краёв карты от границ SVG-документа. Вещественное число не меньше 0 и меньше min(width, height)/2.
    double line_width;          // толщина линий, которыми рисуются автобусные маршруты. Вещественное число в диапазоне от 0 до 100000.
    double stop_radius;         // радиус окружностей, которыми обозначаются остановки. Вещественное число в диапазоне от 0 до 100000.
    int bus_label_font_size;    // размер текста, которым написаны названия автобусных маршрутов. Целое число в диапазоне от 0 до 100000.
    svg::Point bus_label_offset;// смещение надписи с названием маршрута относительно координат конечной остановки на карте. 
                                // Массив из двух элементов типа double. Задаёт значения свойств dx и dy SVG-элемента <text>.
                                // Элементы массива — числа в диапазоне от –100000 до 100000.
    int stop_label_font_size;   // размер текста, которым отображаются названия остановок. Целое число в диапазоне от 0 до 100000.
    svg::Point stop_label_offset; // смещение названия остановки относительно её координат на карте. Массив из двух элементов типа double.
                                  // Задаёт значения свойств dx и dy SVG-элемента <text>. Числа в диапазоне от –100000 до 100000.
    svg::Color underlayer_color;  // цвет подложки под названиями остановок и маршрутов. Формат хранения цвета будет ниже.
    double underlayer_width;      // толщина подложки под названиями остановок и маршрутов. Задаёт значение атрибута stroke-width элемента <text>. Вещественное число в диапазоне от 0 до 100000.
    std::vector<svg::Color> color_palette;    // цветовая палитра. Непустой массив.
};

/* Renderer class */
class MapRenderer {
public:
    explicit MapRenderer(std::ostream& output);

    void SetSettings(Renderer_Settings&& render_settings) {
        render_settings_ = std::move(render_settings);
    }

    const Renderer_Settings& GetSettings() const {
        return render_settings_;
    }

    svg::Document RenderMap(RequestHandler& request_handler) const;

private:
    std::ostream& output_;
    Renderer_Settings render_settings_;
};

template <typename PointInputIt>
inline SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end, 
                                            double max_width, double max_height, double padding) {
    // Если точки поверхности сферы не заданы, вычислять нечего
    if (points_begin == points_end) return;

    // Находим точки с минимальной и максимальной долготой
    const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    // Находим точки с минимальной и максимальной широтой
    const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

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

class Lines : public svg::Drawable {
public:
    Lines(const SphereProjector& projector, const domain::BusType bus_type,
                    const std::vector<const domain::Stop*>& stops, const svg::Color color, const Renderer_Settings& render_settings);

    void Draw(svg::ObjectContainer& container) const override;

private:
    domain::BusType bus_type_;
    svg::Color color_;
    double stroke_width_ = 0;
    std::vector<svg::Point> points_;
};

class BusNames : public svg::Drawable {
public:
    BusNames(const SphereProjector& projector, const domain::BusType bus_type, const std::string& bus_name,
                    const std::vector<const domain::Stop*>& stops, const svg::Color color, const Renderer_Settings& render_settings);

    void Draw(svg::ObjectContainer& container) const override;

private:
    const domain::BusType bus_type_;
    const svg::Color color_;
    const std::string bus_name_;

    svg::Point bus_label_offset_;
    uint32_t font_size_;
    svg::Color underlayer_color_;
    double underlayer_width_;
    std::vector<svg::Point> coordinates_;

    void DrawName(svg::ObjectContainer& container, const svg::Point& pos, const std::string& text) const;
};

class StopPoint : public svg::Drawable {
public:
    StopPoint(const SphereProjector& projector, const domain::Stop* stop, const Renderer_Settings& render_settings);

    void Draw(svg::ObjectContainer& container) const override;

private:
    svg::Point coordinates_;
    double radius_ = 0;
};

class StopName : public svg::Drawable {
public:
    StopName(const SphereProjector& projector, const domain::Stop* stop, const Renderer_Settings& render_settings);

    void Draw(svg::ObjectContainer& container) const override;

private:
    svg::Point coordinates_;
    std::string stop_name_;
    svg::Point stop_label_offset_;
    uint32_t stop_label_font_size_;
    svg::Color underlayer_color_;
    double underlayer_width_;

    void DrawName(svg::ObjectContainer& container, const svg::Point& pos, const std::string& text) const;
};

} // namespace renderer

} // namespace transport_catalogue