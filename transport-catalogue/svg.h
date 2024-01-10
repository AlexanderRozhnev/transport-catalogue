#pragma once

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

struct Point {
    Point() = default;
    Point(double x_, double y_) : x(x_),
                                  y(y_) {
    }
    double x = 0;
    double y = 0;
};

class Rgb {
public:
    Rgb() = default;
    Rgb(uint8_t red_, uint8_t green_, uint8_t blue_) : red(red_),
                                                       green(green_),
                                                       blue(blue_) {}
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

class Rgba {
public:
    Rgba() = default;
    Rgba(uint8_t red_, uint8_t green_, uint8_t blue_, double opacity_) : red(red_),
                                                                         green(green_),
                                                                         blue(blue_),
                                                                         opacity(opacity_) {}
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

// Объявив в заголовочном файле константу со спецификатором inline,
// мы сделаем так, что она будет одной на все единицы трансляции,
// которые подключают этот заголовок.
// В противном случае каждая единица трансляции будет использовать свою копию этой константы
inline const Color NoneColor = std::monostate{};

struct PrintColor {
    std::string operator()(std::monostate) const {
    using namespace std::literals;
        return "none"s;
    }

    std::string operator()(std::string color) const {
        return color;
    }

    std::string operator()(Rgb color) const {
    using namespace std::literals;        
        return "rgb("s + std::to_string(color.red) 
                 + "," + std::to_string(color.green) 
                 + "," + std::to_string(color.blue) 
                 + ")";
    }
    
    std::string operator()(Rgba color) const {
    using namespace std::literals;
        std::ostringstream oss;
        oss << "rgba("s << std::to_string(color.red)
            << "," << std::to_string(color.green)
            << "," << std::to_string(color.blue)
            << "," << color.opacity
            << ")";
        return oss.str();
    }
};

std::ostream& operator<<(std::ostream& output, Color value);

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& output, const StrokeLineCap value);
std::ostream& operator<<(std::ostream& output, const StrokeLineJoin value);

/* Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out_)
        : out(out_) {
    }

    RenderContext(std::ostream& out_, int indent_step_, int indent_ = 0)
        : out(out_)
        , indent_step(indent_step_)
        , indent(indent_) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

/* Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

protected:
    Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};


/* Вспомогательный базовый класс svg::PathProps. 
 * Путь — представленный в виде последовательности различных контуров векторный объект, 
 * который будет содержать свойства, управляющие параметрами заливки и контура.
 */
template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color fill_color) {
        fill_color_ = std::move(fill_color);
        return static_cast<Owner&>(*this);
    }

    Owner& SetStrokeColor(Color stroke_color) {
        stroke_color_ = std::move(stroke_color);
        return static_cast<Owner&>(*this);
    }

    Owner& SetStrokeWidth(double width) {
        width_ = width;
        return static_cast<Owner&>(*this);
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        line_cap_ = std::move(line_cap);
        return static_cast<Owner&>(*this);
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        line_join_ = std::move(line_join);
        return static_cast<Owner&>(*this);
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
    using namespace std::literals;
        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (width_ != 1) {
            out << " stroke-width=\""sv << std::setprecision(6) << width_ << "\""sv;
        }
        if (line_cap_) {
            out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
        }
        if (line_join_) {
            out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
        }
    }

private:
    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    double width_ = 1;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<StrokeLineJoin> line_join_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;
    
    std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;

    Point pos_;
    Point offset_;
    uint32_t size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class ObjectContainer {
public:    
    template <typename T>
    void Add(T object) {
        AddPtr(std::make_unique<T>(std::move(object)));
    }

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

protected:
    ~ObjectContainer() = default;   // ObjectContainer - абстрактный базовый класс, 
                                    // поэтому деструктор должен быть либо protected либо virtual public
};

/* Drawable - интерфейс
 * Класс, наследующийся от Drawable, должен реализовать метод Draw, 
 * чтобы, используя переданный интерфейс Graphics, нарисовать своё изображение.
 */
class Drawable {
public:

    virtual ~Drawable() = default; // Деструктор объявляется виртуальным, т.к. возможно полиморфное удаление 

    virtual void Draw(ObjectContainer& container) const = 0;
};

class Document : public ObjectContainer {
public:
    /* Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
       Пример использования:
       Document doc;
       doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
    */

    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj);

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;

    // Прочие методы и данные, необходимые для реализации класса Document
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg