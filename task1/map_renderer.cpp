// map_renderer.cpp
#include "map_renderer.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

struct Point {
    double x = 0.0;
    double y = 0.0;
};

class SphereProjector {
public:
    SphereProjector(const std::vector<Coordinates>& coords,
                    double width, double height, double padding)
        : padding_(padding) {
        if (coords.empty()) {
            return;
        }

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

        double width_zoom = 0.0;
        if (max_lng_ - min_lng_ != 0.0) {
            width_zoom = usable_w / (max_lng_ - min_lng_);
        }

        double height_zoom = 0.0;
        if (max_lat_ - min_lat_ != 0.0) {
            height_zoom = usable_h / (max_lat_ - min_lat_);
        }

        if (width_zoom == 0.0) {
            zoom_ = height_zoom;
        } else if (height_zoom == 0.0) {
            zoom_ = width_zoom;
        } else {
            zoom_ = std::min(width_zoom, height_zoom);
        }
    }

    Point operator()(Coordinates c) const {
        // x растёт вправо по долготе
        // y растёт вниз, поэтому широту инвертируем
        return {
            (c.lng - min_lng_) * zoom_ + padding_,
            (max_lat_ - c.lat) * zoom_ + padding_
        };
    }

private:
    double padding_ = 0.0;
    double min_lng_ = 0.0, max_lng_ = 0.0;
    double min_lat_ = 0.0, max_lat_ = 0.0;
    double zoom_ = 0.0;
};

static Point ShiftPerp(Point a, Point b, double offset) {
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len == 0.0) {
        return {0.0, 0.0};
    }
    // единичный перпендикуляр (влево относительно направления a->b)
    double nx = -dy / len;
    double ny =  dx / len;
    return {nx * offset, ny * offset};
}

static const char* EmojiFont() {
    // На Windows чаще всего работает Segoe UI Emoji
    return "Segoe UI Emoji, Apple Color Emoji, Noto Color Emoji, sans-serif";
}

// Выбираем стрелку-эмодзи по направлению
static std::string DirEmoji(Point a, Point b) {
    double dx = b.x - a.x;
    double dy = b.y - a.y;

    // если движение больше по X — выбираем 👈/👉
    if (std::fabs(dx) >= std::fabs(dy)) {
        return (dx >= 0) ? u8"👉" : u8"👈";
    }
    // иначе по Y — 👆/👇 (помним что y вниз)
    return (dy <= 0) ? u8"👆" : u8"👇";
}

static Point Lerp(Point p1, Point p2, double t) {
    return {p1.x + (p2.x - p1.x) * t, p1.y + (p2.y - p1.y) * t};
}

// Ключ ребра по указателям Stop (адреса стабильны из deque)
struct EdgeKey {
    const Stop* from = nullptr;
    const Stop* to = nullptr;

    bool operator==(const EdgeKey& other) const noexcept {
        return from == other.from && to == other.to;
    }
};

struct EdgeKeyHasher {
    size_t operator()(const EdgeKey& k) const noexcept {
        // простой комбинированный хеш указателей
        auto h1 = std::hash<const void*>{}(k.from);
        auto h2 = std::hash<const void*>{}(k.to);
        return h1 * 37u + h2;
    }
};

std::string RenderBusSvg(const Bus& bus, double width, double height, double padding) {
    std::ostringstream svg;
    svg << std::fixed << std::setprecision(6);

    // ====== настройки вида ======
    const double top_margin = 50.0;   // место под заголовок
    const double OFFSET = 7.0;        // расстояние между параллельными линиями

    // Разнос эмодзи (высота/ширина эмодзи большая, поэтому разводим И вдоль, И поперёк)
    const double EMOJI_SEP = 12.0;    // разводим 👉 и 🚌 перпендикулярно линии (достаточно и для two_way)
    const double DT_TWOWAY = 0.10;    // дополнительный сдвиг ВДОЛЬ сегмента для двусторонних рёбер

    // ВАЖНО: 👉 ближе к концу, 🚌 ближе к началу (чтобы не стояли рядом)
    const double ARROW_ALONG = 0.80;  // 👉 ближе к концу сегмента
    const double BUS_ALONG   = 0.35;  // 🚌 ближе к началу/середине
    // ===========================

    // Собираем координаты для проекции
    std::vector<Coordinates> coords;
    coords.reserve(bus.stops.size());
    for (const Stop* s : bus.stops) {
        coords.push_back(s->coord);
    }

    // ВАЖНО: проецируем в высоту (height - top_margin),
    // чтобы после сдвига вниз ничего не обрезалось снизу.
    SphereProjector proj(coords, width, height - top_margin, padding);

    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
        << "width=\"" << width << "\" height=\"" << height << "\">\n";

    // Фон
    svg << "  <rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height
        << "\" fill=\"white\" />\n";

    // --- 1) Считаем рёбра ---
    std::unordered_map<EdgeKey, int, EdgeKeyHasher> edge_count;
    edge_count.reserve(bus.stops.size() * 2);

    for (size_t i = 1; i < bus.stops.size(); ++i) {
        EdgeKey e{bus.stops[i - 1], bus.stops[i]};
        ++edge_count[e];
    }

    // --- 2) Рисуем сегменты ---
    // Идея как в макетах:
    // - если есть и A->B и B->A (двустороннее ребро) — рисуем "туда" без смещения,
    //   "обратно" со смещением, чтобы было видно две параллельные линии.
    // - если ребро одностороннее — рисуем обычной линией.
    //
    // Для двусторонних рёбер ОБЯЗАТЕЛЬНО разводим эмодзи ещё и вдоль (t),
    // иначе на двух параллельных линиях эмодзи попадают в одну и ту же зону.
    for (size_t i = 1; i < bus.stops.size(); ++i) {
        const Stop* from = bus.stops[i - 1];
        const Stop* to   = bus.stops[i];

        EdgeKey backward{to, from};
        bool two_way = (edge_count.find(backward) != edge_count.end());

        Point a = proj(from->coord);
        Point b = proj(to->coord);

        // Сдвигаем всю карту вниз, чтобы заголовок не накладывался на карту
        a.y += top_margin;
        b.y += top_margin;

        Point shift{0.0, 0.0};
        bool is_canonical_dir = true;

        if (two_way) {
            // Рисуем параллельные линии:
            // определим "канонический порядок" ребра по адресу указателей:
            // канон = (min(from,to), max(from,to))
            // - каноническое направление (min->max) — без смещения
            // - обратное (max->min) — со смещением
            const Stop* lo = std::min(from, to);
            const Stop* hi = std::max(from, to);
            is_canonical_dir = (from == lo && to == hi);

            if (!is_canonical_dir) {
                shift = ShiftPerp(a, b, OFFSET);
            }
        }

        // линия сегмента
        svg << "  <line x1=\"" << (a.x + shift.x) << "\" y1=\"" << (a.y + shift.y)
            << "\" x2=\"" << (b.x + shift.x) << "\" y2=\"" << (b.y + shift.y)
            << "\" stroke=\"black\" stroke-width=\"3\" "
            << "stroke-linecap=\"round\" stroke-linejoin=\"round\" />\n";

        // ---- эмодзи направления и автобус ----
        // единичный перпендикуляр (для развода эмодзи)
        Point perp = ShiftPerp(a, b, 1.0);

        // Базовые позиции по длине сегмента
        double arrow_t = ARROW_ALONG;
        double bus_t   = BUS_ALONG;

        // Для двусторонних рёбер разнесём ВДОЛЬ сегмента (в разные зоны),
        // иначе на параллельных линиях (туда/обратно) эмодзи совпадут.
        if (two_way) {
            if (is_canonical_dir) {
                arrow_t = std::max(0.05, arrow_t - DT_TWOWAY);
                bus_t   = std::max(0.05, bus_t   - DT_TWOWAY);
            } else {
                arrow_t = std::min(0.95, arrow_t + DT_TWOWAY);
                bus_t   = std::min(0.95, bus_t   + DT_TWOWAY);
            }
        }

        Point arrow_pt = Lerp(a, b, arrow_t);
        Point bus_pt   = Lerp(a, b, bus_t);

        // уводим их на "свою" параллельную линию
        arrow_pt.x += shift.x; arrow_pt.y += shift.y;
        bus_pt.x   += shift.x; bus_pt.y   += shift.y;

        // дополнительно разводим перпендикулярно линии (👉 вверх/вниз от 🚌)
        arrow_pt.x += perp.x * (-EMOJI_SEP);
        arrow_pt.y += perp.y * (-EMOJI_SEP);

        bus_pt.x   += perp.x * (+EMOJI_SEP);
        bus_pt.y   += perp.y * (+EMOJI_SEP);

        // стрелка по направлению
        std::string arrow = DirEmoji(a, b);
        svg << "  <text x=\"" << arrow_pt.x << "\" y=\"" << arrow_pt.y
            << "\" font-size=\"18\" font-family=\"" << EmojiFont() << "\">"
            << arrow << "</text>\n";

        // автобус
        svg << "  <text x=\"" << bus_pt.x << "\" y=\"" << bus_pt.y
            << "\" font-size=\"18\" font-family=\"" << EmojiFont() << "\">"
            << u8"🚌" << "</text>\n";
    }

    // --- 3) Остановки + подписи (подпись один раз на уникальную остановку) ---
    std::unordered_set<const Stop*> drawn;
    drawn.reserve(bus.stops.size());

    for (const Stop* s : bus.stops) {
        Point p = proj(s->coord);
        p.y += top_margin;

        svg << "  <circle cx=\"" << p.x << "\" cy=\"" << p.y
            << "\" r=\"6\" fill=\"yellow\" stroke=\"black\" stroke-width=\"2\" />\n";

        if (drawn.insert(s).second) {
            // рядом с остановкой эмодзи 🚏
            svg << "  <text x=\"" << (p.x + 10) << "\" y=\"" << (p.y + 6)
                << "\" font-size=\"16\" font-family=\"" << EmojiFont() << "\">"
                << u8"🚏" << "</text>\n";

            // подпись остановки
            svg << "  <text x=\"" << (p.x + 30) << "\" y=\"" << (p.y - 10)
                << "\" font-size=\"14\" font-family=\"Verdana\" fill=\"black\">"
                << s->name << "</text>\n";
        }
    }

    // Заголовок (выше карты; под него зарезервирован top_margin)
    svg << "  <text x=\"" << padding << "\" y=\"" << 30
        << "\" font-size=\"22\" font-family=\"Verdana\" fill=\"black\">"
        << "Bus: " << bus.name << "</text>\n";

    svg << "</svg>\n";
    return svg.str();
}
