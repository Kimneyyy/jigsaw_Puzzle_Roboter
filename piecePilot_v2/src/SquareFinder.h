#ifndef SQUAREFINDER_H
#define SQUAREFINDER_H

#pragma once

#include <opencv2/core.hpp>
#include <vector>

class SquareFinder
{
public:
    // Score 0..1: how well corners[0..3] (in this order) form a rectangle
    // Returns 0 if size != 4 or degenerate.
    static double rectangleScore01(const std::vector<cv::Point>& corners);

private:
    static double clamp01(double x);
    static double len(const cv::Point2f& v);
    static double dotp(const cv::Point2f& a, const cv::Point2f& b);
    static double crossp(const cv::Point2f& a, const cv::Point2f& b);

    // gauss-like falloff around 0: exp(-(x^2)/(2*sigma^2))
    static double gaussian01(double x, double sigma);
};


#endif // SQUAREFINDER_H