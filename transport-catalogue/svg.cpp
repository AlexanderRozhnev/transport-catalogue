#include "svg.h"

#include "svg.h"

#include <unordered_map>

namespace svg {

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, Color value) {
    std::string s = std::visit(PrintColor{}, value); 
    output << s;
    return output;
}

std::ostream &operator<<(std::ostream &output, const StrokeLineCap value) {
    const std::unordered_map<StrokeLineCap, std::string> line_caps = {
        {StrokeLineCap::BUTT, "butt"s},
        {StrokeLineCap::ROUND, "round"s},
        {StrokeLineCap::SQUARE, "square"s},
    };
    if (line_caps.count(value)) {
        output << line_caps.at(value);
    }
    return output;
}    

std::ostream& operator<<(std::ostream& output, const StrokeLineJoin value) {
using namespace std::literals;
    const std::unordered_map<StrokeLineJoin, std::string> line_joins = {
        {StrokeLineJoin::ARCS, "arcs"s},
        {StrokeLineJoin::BEVEL, "bevel"s},
        {StrokeLineJoin::MITER, "miter"s},
        {StrokeLineJoin::MITER_CLIP, "miter-clip"s},
        {StrokeLineJoin::ROUND, "round"s},
    };
    if (line_joins.count(value)) {
        output << line_joins.at(value);
    }
    return output;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Polyline -----------------
Polyline &Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext &context) const {
    std::ostringstream s;

    // format: <polyline points="0,100 50,25 50,75 100,0" />
    s << "<polyline points=\""sv;
    if (!points_.empty()) {
        auto it = points_.cbegin();
        s << it->x << "," << it->y;
        ++it;
        for (; it != points_.end(); ++it) {
            s << " " << it->x << "," << it->y;
        }
    }
    s << "\""sv;
    RenderAttrs(s);
    s << "/>"sv;
    context.out << s.str();
}

// ---------- Text -----------------
Text &Text::SetPosition(Point pos) {
    pos_ = std::move(pos);
    return *this;
}

Text &Text::SetOffset(Point offset) {
    offset_ = std::move(offset);
    return *this;
}

Text &Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text &Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text &Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text &Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext &context) const {
    auto& out = context.out;

    if (data_.empty()) return;

    // format: <text dx="10" dy="20" x="100" y="20" font-size="12px" font-family="Arial, Helvetica, 
    // sans-serif" font-weight="normal">Normal text</text>
    out << "<text"sv;
    RenderAttrs(context.out);
    out << " x=\""sv << pos_.x << "\""sv;
    out << " y=\""sv << pos_.y << "\""sv;
    out << " dx=\""sv << offset_.x << "\""sv;
    out << " dy=\""sv << offset_.y << "\""sv;
    out << " font-size=\"" << size_ << "\"";
    if (!font_family_.empty()) out << " font-family=\"" << font_family_ << "\"";
    if (!font_weight_.empty()) out << " font-weight=\"" << font_weight_ << "\"";
    out << ">"sv << data_ << "</text>"sv;
}

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(move(obj));
}

void Document::Render(std::ostream &out) const {

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

    RenderContext context{out, 0, 2};
    for (const auto& ptr : objects_) {
        ptr->Render(context);
    }

    out << "</svg>"sv;
}

} // namespace svg