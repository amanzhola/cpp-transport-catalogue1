// geo.h
#pragma once

/**************************************************************************************************
 * ADDED:
 *   - namespace transport_catalogue::geo
 *   - В конце файла глобальные алиасы Coordinates и ComputeDistance для совместимости
 *
 * ❌ REMOVED:
 *   - Глобальное объявление Coordinates/ComputeDistance (теперь внутри geo)
 **************************************************************************************************/

#include <cmath>

namespace transport_catalogue::geo {

struct Coordinates {
    double lat;
    double lng;
    bool operator==(const Coordinates& other) const {
        return lat == other.lat && lng == other.lng;
    }
    bool operator!=(const Coordinates& other) const {
        return !(*this == other);
    }
};

inline double ComputeDistance(Coordinates from, Coordinates to) {
    using namespace std;
    if (from == to) {
        return 0;
    }
    static const double dr = 3.1415926535 / 180.;
    return acos(sin(from.lat * dr) * sin(to.lat * dr)
                + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
        * 6371000;
}

} // namespace transport_catalogue::geo

// COMPAT: старые глобальные имена для Task2-тестов
using Coordinates = transport_catalogue::geo::Coordinates;
using transport_catalogue::geo::ComputeDistance;
