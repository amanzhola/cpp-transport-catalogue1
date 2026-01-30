#pragma once

#include <cmath>

namespace tc::geo {

struct Coordinates {
    double lat = 0.0;
    double lng = 0.0;

    bool operator==(const Coordinates& other) const {
        return lat == other.lat && lng == other.lng;
    }
    bool operator!=(const Coordinates& other) const {
        return !(*this == other);
    }
};

inline double ComputeDistance(Coordinates from, Coordinates to) {
    if (from == to) {
        return 0.0;
    }

    static constexpr double kPi = 3.1415926535;
    static constexpr double kEarthRadius = 6371000.0;
    static constexpr double kDegToRad = kPi / 180.0;

    const double lat1 = from.lat * kDegToRad;
    const double lat2 = to.lat * kDegToRad;
    const double d_lng = std::abs(from.lng - to.lng) * kDegToRad;

    return std::acos(std::sin(lat1) * std::sin(lat2)
                   + std::cos(lat1) * std::cos(lat2) * std::cos(d_lng))
         * kEarthRadius;
}

}  // namespace tc::geo
