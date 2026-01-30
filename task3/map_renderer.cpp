// map_renderer.cpp
#include "map_renderer.h"

/**************************************************************************************************
 * Неймспейсы:
 * - transport_catalogue::render         — публичные функции (RenderBusSvg / RenderStopSvg)
 * - transport_catalogue::render::detail — внутренняя кухня (Point, Projector, helpers)
 *
 * Исправления:
 * - Добавлены DrawStops + DrawHeader (уменьшает дублирование)
 * - top_margin динамический в RenderStopSvg (шапка не липнет, снизу не режется)
 * - увеличен развод двухсторонних линий (two-way), чтобы "туда/обратно" не слипались
 **************************************************************************************************/

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace transport_catalogue::render {

namespace detail {

using transport_catalogue::domain::Stop;
using transport_catalogue::domain::Bus;
using transport_catalogue::geo::Coordinates;

struct Point {
    double x = 0.0;
    double y = 0.0;
};

class SphereProjector {
public:
    SphereProjector(const std::vector<Coordinates>& coords,
                    double width, double height, double padding)
        : padding_(padding) {
        if (coords.empty()) return;

        min_lng_ = max_lng_ = coords[0].lng;
        min_lat_ = max_lat_ = coords[0].lat;

        for (const auto& c : coords) {
            min_lng_ = std::min(min_lng_, c.lng);
            max_lng_ = std::max(max_lng_, c.lng);
            min_lat_ = std::min(min_lat_, c.lat);
            max_lat_ = std::max(max_lat_, c.lat);
        }

        const double usable_w = width - 2 * padding_;
        const double usable_h = height - 2 * padding_;

        const double zx = (max_lng_ - min_lng_ != 0.0) ? usable_w / (max_lng_ - min_lng_) : 0.0;
        const double zy = (max_lat_ - min_lat_ != 0.0) ? usable_h / (max_lat_ - min_lat_) : 0.0;

        if (zx == 0.0) zoom_ = zy;
        else if (zy == 0.0) zoom_ = zx;
        else zoom_ = std::min(zx, zy);
    }

    Point operator()(Coordinates c) const {
        return {
            (c.lng - min_lng_) * zoom_ + padding_,
            (max_lat_ - c.lat) * zoom_ + padding_
        };
    }

private:
    double padding_;
    double min_lng_ = 0.0, max_lng_ = 0.0;
    double min_lat_ = 0.0, max_lat_ = 0.0;
    double zoom_ = 0.0;
};

static Point ShiftPerp(Point a, Point b, double offset) {
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len == 0.0) return {0.0, 0.0};

    // единичный перпендикуляр (влево относительно a->b)
    const double nx = -dy / len;
    const double ny =  dx / len;
    return {nx * offset, ny * offset};
}

static Point Lerp(Point a, Point b, double t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

static const char* EmojiFont() {
    return "Segoe UI Emoji, Apple Color Emoji, Noto Color Emoji, sans-serif";
}

static std::string DirEmoji(Point a, Point b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;

    if (std::fabs(dx) >= std::fabs(dy)) {
        return (dx >= 0) ? u8"👉" : u8"👈";
    }
    return (dy <= 0) ? u8"👆" : u8"👇";
}

struct EdgeKey {
    const Stop* from = nullptr;
    const Stop* to = nullptr;
    bool operator==(const EdgeKey& o) const noexcept { return from == o.from && to == o.to; }
};

struct EdgeKeyHasher {
    size_t operator()(const EdgeKey& k) const noexcept {
        auto h1 = std::hash<const void*>{}(k.from);
        auto h2 = std::hash<const void*>{}(k.to);
        return h1 * 37u + h2;
    }
};

static const std::vector<std::string>& ColorPalette() {
    static const std::vector<std::string> p = {
        "red","green","blue","orange","purple","brown","magenta","teal","navy","gold"
    };
    return p;
}

/**************************************************************************************************
 * RouteDrawStyle — единый набор параметров рисования маршрута
 **************************************************************************************************/
struct RouteDrawStyle {
    const char* stroke_color = "black";
    double stroke_width = 3.0;

    double emoji_sep = 12.0;
    double arrow_along = 0.80;
    double bus_along   = 0.35;

    // 🔥 увеличили базовый отступ для two-way
    double offset_twoway = 10.0;
    double dt_twoway     = 0.10;

    // внешний сдвиг (например для stop-map: раздвигаем маршруты между собой)
    Point extra_shift{0.0, 0.0};
};

/**************************************************************************************************
 * DrawBusSegments — единая реализация сегментов + эмодзи
 **************************************************************************************************/
static void DrawBusSegments(std::ostringstream& svg,
                            const Bus& bus,
                            const SphereProjector& proj,
                            double top_margin,
                            const RouteDrawStyle& st) {
    if (bus.stops.size() < 2) return;

    std::unordered_map<EdgeKey, int, EdgeKeyHasher> edges;
    edges.reserve(bus.stops.size() * 2);

    for (size_t i = 1; i < bus.stops.size(); ++i) {
        ++edges[{bus.stops[i - 1], bus.stops[i]}];
    }

    for (size_t i = 1; i < bus.stops.size(); ++i) {
        const Stop* from = bus.stops[i - 1];
        const Stop* to   = bus.stops[i];

        Point a = proj(from->coord);
        Point b = proj(to->coord);
        a.y += top_margin;
        b.y += top_margin;

        const Point perp = ShiftPerp(a, b, 1.0);

        // two-way?
        const bool two_way = edges.find({to, from}) != edges.end();

        // каноническое направление по указателям
        const Stop* lo = std::min(from, to);
        const Stop* hi = std::max(from, to);
        const bool canonical = (from == lo && to == hi);

        Point shift = st.extra_shift;

        if (two_way && !canonical) {
            // 🔥 Разводим сильнее: базовый offset + зависимость от толщины линии
            const double tw = st.offset_twoway + st.stroke_width * 1.5;
            const Point tw_shift = ShiftPerp(a, b, tw);
            shift.x += tw_shift.x;
            shift.y += tw_shift.y;
        }

        // линия
        svg << "  <line x1=\"" << a.x + shift.x << "\" y1=\"" << a.y + shift.y
            << "\" x2=\"" << b.x + shift.x << "\" y2=\"" << b.y + shift.y
            << "\" stroke=\"" << st.stroke_color
            << "\" stroke-width=\"" << st.stroke_width
            << "\" stroke-linecap=\"round\" stroke-linejoin=\"round\" />\n";

        // точки для эмодзи
        double at = st.arrow_along;
        double bt = st.bus_along;

        if (two_way) {
            if (canonical) {
                at = std::max(0.05, at - st.dt_twoway);
                bt = std::max(0.05, bt - st.dt_twoway);
            } else {
                at = std::min(0.95, at + st.dt_twoway);
                bt = std::min(0.95, bt + st.dt_twoway);
            }
        }

        Point arrow = Lerp(a, b, at);
        Point buspt = Lerp(a, b, bt);

        arrow.x += shift.x - perp.x * st.emoji_sep;
        arrow.y += shift.y - perp.y * st.emoji_sep;

        buspt.x += shift.x + perp.x * st.emoji_sep;
        buspt.y += shift.y + perp.y * st.emoji_sep;

        svg << "  <text x=\"" << arrow.x << "\" y=\"" << arrow.y
            << "\" font-size=\"18\" font-family=\"" << EmojiFont() << "\">"
            << DirEmoji(a, b) << "</text>\n";

        svg << "  <text x=\"" << buspt.x << "\" y=\"" << buspt.y
            << "\" font-size=\"18\" font-family=\"" << EmojiFont() << "\">"
            << u8"🚌" << "</text>\n";
    }
}

/**************************************************************************************************
 * DrawBusOnStopMap — рисует маршрут на карте остановки (цвет + развод маршрутов)
 **************************************************************************************************/
static void DrawBusOnStopMap(std::ostringstream& svg,
                             const Bus& bus,
                             const SphereProjector& proj,
                             const std::string& color,
                             double top_margin,
                             double offset_index) {
    if (bus.stops.size() < 2) return;

    const double BASE_OFFSET = 3.5;

    // найдём perp по первому ненулевому сегменту
    Point perp{0.0, 0.0};
    for (size_t i = 1; i < bus.stops.size(); ++i) {
        Point a = proj(bus.stops[i - 1]->coord);
        Point b = proj(bus.stops[i]->coord);
        a.y += top_margin;
        b.y += top_margin;
        const Point p = ShiftPerp(a, b, 1.0);
        if (p.x != 0.0 || p.y != 0.0) {
            perp = p;
            break;
        }
    }

    RouteDrawStyle st;
    st.stroke_color = color.c_str();
    st.stroke_width = 4.0;
    st.emoji_sep = 10.0;

    st.extra_shift = {perp.x * (BASE_OFFSET * offset_index),
                      perp.y * (BASE_OFFSET * offset_index)};

    DrawBusSegments(svg, bus, proj, top_margin, st);
}

/**************************************************************************************************
 * DrawStops — рисует остановки + подписи
 **************************************************************************************************/
static void DrawStops(std::ostringstream& svg,
                      const std::unordered_set<const Stop*>& stops,
                      const SphereProjector& proj,
                      double top_margin,
                      const Stop* highlight_stop = nullptr,
                      bool yellow_mode = false) {
    for (const Stop* s : stops) {
        Point p = proj(s->coord);
        p.y += top_margin;

        const bool hi = (s == highlight_stop);

        const double r = hi ? 9.0 : (yellow_mode ? 6.0 : 5.0);
        const std::string fill = hi ? "yellow" : (yellow_mode ? "yellow" : "white");
        const std::string stroke = hi ? "red" : "black";
        const double sw = hi ? 3.0 : 2.0;

        svg << "  <circle cx=\"" << p.x << "\" cy=\"" << p.y
            << "\" r=\"" << r << "\" fill=\"" << fill
            << "\" stroke=\"" << stroke << "\" stroke-width=\"" << sw << "\" />\n";

        svg << "  <text x=\"" << (p.x + 10) << "\" y=\"" << (p.y + 6)
            << "\" font-size=\"16\" font-family=\"" << EmojiFont() << "\">"
            << u8"🚏" << "</text>\n";

        svg << "  <text x=\"" << (p.x + 30) << "\" y=\"" << (p.y - 10)
            << "\" font-size=\"14\" font-family=\"Verdana\" fill=\"black\">"
            << s->name << "</text>\n";
    }
}

/**************************************************************************************************
 * DrawHeader — шапка Stop SVG (подложка + заголовок + легенда)
 **************************************************************************************************/
static void DrawHeader(std::ostringstream& svg,
                       const Stop& stop,
                       const std::vector<const Bus*>& buses,
                       double width,
                       double padding,
                       double header_height) {
    const double rect_x = padding - 10.0;
    const double rect_y = 10.0;
    const double rect_w = width - 2 * padding + 20.0;
    const double rect_h = header_height;

    svg << "  <rect x=\"" << rect_x << "\" y=\"" << rect_y
        << "\" width=\"" << rect_w << "\" height=\"" << rect_h
        << "\" fill=\"white\" opacity=\"0.92\" />\n";

    double x = padding;
    double y = 30.0;

    svg << "  <text x=\"" << x << "\" y=\"" << y
        << "\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\">"
        << "Stop: " << stop.name << "</text>\n";

    y += 22.0;
    svg << "  <text x=\"" << x << "\" y=\"" << y
        << "\" font-size=\"14\" font-family=\"Verdana\" fill=\"black\">"
        << "Routes shown in this SVG:</text>\n";

    y += 20.0;

    const auto& pal = ColorPalette();
    for (size_t i = 0; i < buses.size(); ++i) {
        const std::string& color = pal[i % pal.size()];

        svg << "  <rect x=\"" << x << "\" y=\"" << (y - 12)
            << "\" width=\"14\" height=\"14\" fill=\"" << color
            << "\" stroke=\"black\" stroke-width=\"1\" />\n";

        svg << "  <text x=\"" << (x + 20) << "\" y=\"" << y
            << "\" font-size=\"14\" font-family=\"Verdana\" fill=\"black\">"
            << "Bus " << buses[i]->name << "</text>\n";

        y += 18.0;
    }
}

} // namespace detail

// ============================== PUBLIC API ==============================

std::string RenderBusSvg(const transport_catalogue::domain::Bus& bus,
                         double width, double height, double padding) {
    using namespace detail;

    std::ostringstream svg;
    svg << std::fixed << std::setprecision(6);

    const double top_margin = 70.0;

    std::vector<Coordinates> coords;
    coords.reserve(bus.stops.size());
    for (const Stop* s : bus.stops) {
        coords.push_back(s->coord);
    }

    SphereProjector proj(coords, width, height - top_margin, padding);

    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
        << "width=\"" << width << "\" height=\"" << height << "\">\n";

    svg << "  <rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height
        << "\" fill=\"white\" />\n";

    RouteDrawStyle st;
    st.stroke_color = "black";
    st.stroke_width = 3.0;
    st.emoji_sep    = 12.0;

    DrawBusSegments(svg, bus, proj, top_margin, st);

    std::unordered_set<const Stop*> uniq;
    uniq.reserve(bus.stops.size());
    for (const Stop* s : bus.stops) {
        uniq.insert(s);
    }

    DrawStops(svg, uniq, proj, top_margin, nullptr, true);

    svg << "  <text x=\"" << padding << "\" y=\"" << 30
        << "\" font-size=\"22\" font-family=\"Verdana\" fill=\"black\">"
        << "Bus: " << bus.name << "</text>\n";

    svg << "</svg>\n";
    return svg.str();
}

std::string RenderStopSvg(const transport_catalogue::domain::Stop& stop,
                          const std::vector<const transport_catalogue::domain::Bus*>& buses,
                          double width, double height, double padding) {
    using namespace detail;

    std::ostringstream svg;
    svg << std::fixed << std::setprecision(6);

    constexpr double kHeaderTopY       = 30.0;
    constexpr double kTitleLineHeight  = 22.0;
    constexpr double kSecondLineHeight = 20.0;
    constexpr double kLegendLineStep   = 18.0;
    constexpr double kGapHeaderToMap   = 25.0;

    const double header_height =
        kHeaderTopY + kTitleLineHeight + kSecondLineHeight + buses.size() * kLegendLineStep;

    const double top_margin = header_height + kGapHeaderToMap;

    std::vector<Coordinates> coords;
    coords.reserve(256);
    coords.push_back(stop.coord);

    for (const Bus* b : buses) {
        for (const Stop* s : b->stops) {
            coords.push_back(s->coord);
        }
    }

    SphereProjector proj(coords, width, height - top_margin, padding);

    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
        << "width=\"" << width << "\" height=\"" << height << "\">\n";

    svg << "  <rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height
        << "\" fill=\"white\" />\n";

    const auto& pal = ColorPalette();
    for (size_t i = 0; i < buses.size(); ++i) {
        const Bus* bus = buses[i];
        const std::string& color = pal[i % pal.size()];
        DrawBusOnStopMap(svg, *bus, proj, color, top_margin, static_cast<double>(i));
    }

    std::unordered_set<const Stop*> uniq;
    uniq.reserve(512);
    uniq.insert(&stop);

    for (const Bus* b : buses) {
        for (const Stop* s : b->stops) {
            uniq.insert(s);
        }
    }

    DrawStops(svg, uniq, proj, top_margin, &stop, false);
    DrawHeader(svg, stop, buses, width, padding, header_height);

    svg << "</svg>\n";
    return svg.str();
}

} // namespace transport_catalogue::render
