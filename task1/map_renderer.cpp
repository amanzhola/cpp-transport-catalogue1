#include "map_renderer.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace tc::render {

namespace {

struct Point {
    double x = 0.0;
    double y = 0.0;
};

struct SphereProjector {
    double min_lng = 0.0;
    double max_lat = 0.0;
    double zoom = 0.0;
    double padding = 0.0;

    Point operator()(tc::geo::Coordinates c) const {
        return {(c.lng - min_lng) * zoom + padding,
                (max_lat - c.lat) * zoom + padding};
    }
};

static std::string EscapeXml(std::string_view s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  r += "&amp;";  break;
            case '<':  r += "&lt;";   break;
            case '>':  r += "&gt;";   break;
            case '"':  r += "&quot;"; break;
            case '\'': r += "&apos;"; break;
            default:   r += c;        break;
        }
    }
    return r;
}

static void DrawTextWithUnderlayer(std::ostream& out,
                                   double x, double y,
                                   std::string_view text,
                                   int font_size,
                                   std::string_view fill_color,
                                   std::string_view underlayer_color,
                                   double underlayer_width) {
    const std::string esc = EscapeXml(text);

    out << "<text x=\"" << x << "\" y=\"" << y << "\" "
        << "font-size=\"" << font_size << "\" "
        << "font-family=\"Verdana, Segoe UI Emoji, Apple Color Emoji, Noto Color Emoji\" "
        << "fill=\"" << underlayer_color << "\" "
        << "stroke=\"" << underlayer_color << "\" "
        << "stroke-width=\"" << underlayer_width << "\" "
        << "stroke-linecap=\"round\" stroke-linejoin=\"round\">"
        << esc << "</text>\n";

    out << "<text x=\"" << x << "\" y=\"" << y << "\" "
        << "font-size=\"" << font_size << "\" "
        << "font-family=\"Verdana, Segoe UI Emoji, Apple Color Emoji, Noto Color Emoji\" "
        << "fill=\"" << fill_color << "\">"
        << esc << "</text>\n";
}

static void DrawEmoji(std::ostream& out, double x, double y,
                      std::string_view emoji, double font_size) {
    out << "<text x=\"" << x << "\" y=\"" << y << "\" "
        << "font-size=\"" << font_size << "\" "
        << "text-anchor=\"middle\" dominant-baseline=\"middle\" "
        << "font-family=\"Segoe UI Emoji, Apple Color Emoji, Noto Color Emoji, Verdana\">"
        << EscapeXml(emoji) << "</text>\n";
}

}  // namespace

void RenderMapSvg(const tc::catalogue::TransportCatalogue& db,
                  std::ostream& out,
                  const RenderSettings& s) {
    const auto& all_buses = db.GetBuses();

    // stops used by buses
    std::vector<const tc::catalogue::Stop*> stops;
    std::unordered_set<const tc::catalogue::Stop*> used;
    used.reserve(1024);

    for (const auto& b : all_buses) {
        for (const auto* st : b.stops) {
            if (st && used.insert(st).second) stops.push_back(st);
        }
    }

    out << std::fixed << std::setprecision(6);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
        << "width=\"" << s.width << "\" height=\"" << s.height << "\">\n";

    if (stops.empty()) {
        out << "</svg>\n";
        return;
    }

    double min_lng = std::numeric_limits<double>::infinity();
    double max_lng = -std::numeric_limits<double>::infinity();
    double min_lat = std::numeric_limits<double>::infinity();
    double max_lat = -std::numeric_limits<double>::infinity();

    for (const auto* st : stops) {
        min_lng = std::min(min_lng, st->coord.lng);
        max_lng = std::max(max_lng, st->coord.lng);
        min_lat = std::min(min_lat, st->coord.lat);
        max_lat = std::max(max_lat, st->coord.lat);
    }

    const double width_zoom =
        (max_lng - min_lng == 0.0) ? 0.0 : (s.width - 2 * s.padding) / (max_lng - min_lng);
    const double height_zoom =
        (max_lat - min_lat == 0.0) ? 0.0 : (s.height - 2 * s.padding) / (max_lat - min_lat);

    double zoom = 0.0;
    if (width_zoom == 0.0) zoom = height_zoom;
    else if (height_zoom == 0.0) zoom = width_zoom;
    else zoom = std::min(width_zoom, height_zoom);

    SphereProjector proj{min_lng, max_lat, zoom, s.padding};

    // sort buses by name
    std::vector<const tc::catalogue::Bus*> buses;
    buses.reserve(all_buses.size());
    for (const auto& b : all_buses) {
        if (!b.stops.empty()) buses.push_back(&b);
    }
    std::sort(buses.begin(), buses.end(),
              [](const auto* a, const auto* b){ return a->name < b->name; });

    const char* colors[] = {"red","green","blue","orange","purple","brown","teal","magenta"};
    const int color_count = 8;

    // -------- ROUTES + 🚌 LABELS ----------
    int color_idx = 0;
    for (const auto* bus : buses) {
        const char* color = colors[color_idx % color_count];

        std::vector<const tc::catalogue::Stop*> route;
        route.reserve(bus->stops.size() * 2 + 1);

        for (const auto* st : bus->stops) if (st) route.push_back(st);
        if (route.size() < 2) { ++color_idx; continue; }

        if (bus->is_roundtrip) {
            if (route.front() != route.back()) route.push_back(route.front());
        } else {
            const size_t base = route.size();
            for (size_t i = base; i-- > 1; ) {
                route.push_back(route[i - 1]);
            }
        }

        out << "<polyline fill=\"none\" "
            << "stroke=\"" << color << "\" "
            << "stroke-width=\"" << s.line_width << "\" "
            << "stroke-opacity=\"" << s.line_opacity << "\" "
            << "stroke-linecap=\"round\" stroke-linejoin=\"round\" "
            << "points=\"";

        for (const auto* st : route) {
            const Point p = proj(st->coord);
            out << p.x << "," << p.y << " ";
        }
        out << "\" />\n";

        // 🚌 label near start (with underlayer)
        if (!route.empty() && route.front()) {
            const Point p = proj(route.front()->coord);
            const std::string label = std::string("🚌 ") + bus->name;
            DrawTextWithUnderlayer(out, p.x + 6, p.y - 6,
                                   label, s.label_font_size,
                                   color, s.underlayer_color, s.underlayer_width);
        }

        // 🚌 label near end (optional)
        if (s.draw_bus_end_label && route.size() >= 2 &&
            route.back() && route.back() != route.front()) {
            const Point p = proj(route.back()->coord);
            const std::string label = std::string("🚌 ") + bus->name;
            DrawTextWithUnderlayer(out, p.x + 6, p.y - 6,
                                   label, s.label_font_size,
                                   color, s.underlayer_color, s.underlayer_width);
        }

        ++color_idx;
    }

    // -------- STOPS as 🚏 + names ----------
    for (const auto* st : stops) {
        const Point p = proj(st->coord);

        // 🚏 icon
        const double emoji_size = std::max(12.0, s.stop_radius * 3.2);
        DrawEmoji(out, p.x, p.y, "🚏", emoji_size);

        // stop name
        if (s.draw_stop_labels) {
            DrawTextWithUnderlayer(out, p.x + 14, p.y + 6,
                                   st->name, s.label_font_size - 2,
                                   "black", s.underlayer_color, s.underlayer_width);
        }
    }

    out << "</svg>\n";
}

}  // namespace tc::render
