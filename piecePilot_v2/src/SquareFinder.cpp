#include "SquareFinder.h"

#include <cmath>
#include <algorithm>

double SquareFinder::clamp01(double x)
{
    return std::max(0.0, std::min(1.0, x));
}

double SquareFinder::len(const cv::Point2f& v)
{
    return std::sqrt(v.x*v.x + v.y*v.y);
}

double SquareFinder::dotp(const cv::Point2f& a, const cv::Point2f& b)
{
    return a.x*b.x + a.y*b.y;
}

double SquareFinder::crossp(const cv::Point2f& a, const cv::Point2f& b)
{
    return a.x*b.y - a.y*b.x;
}

double SquareFinder::gaussian01(double x, double sigma)
{
    return std::exp(-(x*x) / (2.0*sigma*sigma));
}

double SquareFinder::rectangleScore01(const std::vector<cv::Point>& corners)
{
    if (corners.size() != 4) return 0.0;

    cv::Point2f p0 = corners[0], p1 = corners[1], p2 = corners[2], p3 = corners[3];

    // Edge vectors in given order
    cv::Point2f e0 = p1 - p0;
    cv::Point2f e1 = p2 - p1;
    cv::Point2f e2 = p3 - p2;
    cv::Point2f e3 = p0 - p3;

    double L0 = len(e0), L1 = len(e1), L2 = len(e2), L3 = len(e3);

    // Reject degenerate shapes
    const double eps = 1e-6;
    if (L0 < eps || L1 < eps || L2 < eps || L3 < eps) return 0.0;

    // 1) Right angles
    auto angleScoreAt = [&](const cv::Point2f& a, const cv::Point2f& b) {
        double c = dotp(a,b) / (len(a)*len(b));        // -1..1
        c = std::max(-1.0, std::min(1.0, c));
        // want cos ~ 0
        return gaussian01(c, 0.15);
    };

    double sA0 = angleScoreAt(e0, e1);
    double sA1 = angleScoreAt(e1, e2);
    double sA2 = angleScoreAt(e2, e3);
    double sA3 = angleScoreAt(e3, e0);
    double angleScore = std::pow(sA0*sA1*sA2*sA3, 1.0/4.0);

    // 2) Parallel opposite sides
    auto parallelScore = [&](const cv::Point2f& a, const cv::Point2f& b) {
        double s = std::abs(crossp(a,b)) / (len(a)*len(b)); // |sin(theta)|
        return gaussian01(s, 0.10);
    };

    double sP0 = parallelScore(e0, e2);
    double sP1 = parallelScore(e1, e3);
    double parallelScoreAll = std::sqrt(sP0*sP1);

    // 3) Opposite side lengths equal
    auto lengthPairScore = [&](double a, double b) {
        double denom = std::max(a, b);
        double rel = std::abs(a - b) / denom; // relative error
        return gaussian01(rel, 0.08);
    };

    double sL0 = lengthPairScore(L0, L2);
    double sL1 = lengthPairScore(L1, L3);
    double lengthScoreAll = std::sqrt(sL0*sL1);

    // 4) Area sanity (avoid almost-line)
    auto signedArea = [&](cv::Point2f a, cv::Point2f b, cv::Point2f c, cv::Point2f d) {
        return 0.5 * (a.x*b.y - a.y*b.x +
                      b.x*c.y - b.y*c.x +
                      c.x*d.y - d.y*c.x +
                      d.x*a.y - d.y*a.x);
    };

    double area = std::abs(signedArea(p0,p1,p2,p3));
    double per  = L0 + L1 + L2 + L3;
    double areaNorm = area / (per*per + 1e-9);
    double areaScore = clamp01(areaNorm * 50.0);

    double score = angleScore * parallelScoreAll * lengthScoreAll * areaScore;
    return clamp01(score);
}
