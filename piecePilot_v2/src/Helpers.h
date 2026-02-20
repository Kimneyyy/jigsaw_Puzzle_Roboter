#ifndef HELPERS_H
#define HELPERS_H

#include <opencv2/core.hpp>
#include <cmath>
#include <algorithm>

class Helpers
{
public:
    static inline int wrapIndex(int i, int n){
        // liefert immer 0..n-1, auch wenn i negativ ist
        i %= n;
        if (i < 0) i += n;
        return i;
    }

    // Winkel an einem Punkt berechnen, gegeben drei Punkte: Winkelpunkt, Punkt davor und Punkt danach
    static inline float getAngle(cv::Point anglePoint, cv::Point b, cv::Point c) {
        cv::Point2f u = b - anglePoint;
        cv::Point2f v = c - anglePoint;

        float du = std::hypot(u.x, u.y);
        float dv = std::hypot(v.x, v.y);
        if (du < 1e-6f || dv < 1e-6f) return 0.0f;

        float dot = u.x * v.x + u.y * v.y;
        float cosang = dot / (du * dv);

        cosang = std::max(-1.0f, std::min(1.0f, cosang));

        return std::acos(cosang); // Radiant (0..pi)
    }

    static inline float getAngleDeg(cv::Point anglePoint, cv::Point b, cv::Point c) {
        float rad = getAngle(anglePoint, b, c);
        return rad * 180.0f / (float)CV_PI;
    }


    static double getDistance(cv::Point a, cv::Point b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        return std::sqrt(dx*dx + dy*dy);  // a² + b² = c² => c = √(a² + b²)
    }


};

#endif // HELPERS_H