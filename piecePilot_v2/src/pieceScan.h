#ifndef PieceScan_H
#define PieceScan_H
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

class PieceInfo;
class PieceEdgeProfile;

class PieceScan
{
private:
    /* data */
public:
    PieceScan(/* args */);
    ~PieceScan();

    int scanPiece(PieceInfo* piece, const std::string& imagePath, const std::string& outputDir);

    void saveDebugImage(std::string path, std::string name, cv::Mat img);
    void saveDebugImage(std::string path, std::string name, cv::Mat img, std::vector<cv::Point> contour);
    void saveDebugCSV_EdgeProfile(PieceEdgeProfile* edgeProfile);

    cv::Rect findBoundingBox(const cv::Mat& img, std::vector<cv::Point> contour, std::string outputDir = "");

    std::vector<std::vector<cv::Point>> findContours(const cv::Mat& mask);
    std::vector<cv::Point> findBiggestContour(const std::vector<std::vector<cv::Point>>& contours);

    cv::Point findNextCornerAlongLine(cv::Point p0, std::vector<cv::Point> contour);
    cv::Point findNextCornerAlongLine(cv::Point pMitte, cv::Point pDavor, cv::Point pNachfolger, std::vector<cv::Point> contour);
    std::vector<cv::Point> sortCorners(std::vector<cv::Point> corners);

    cv::Point2f findFirstMaskHitAlongLine(const cv::Mat& mask, cv::Point2f p0, cv::Point2f p1);

    static int nearestContourIndex(const std::vector<cv::Point>& contour, const cv::Point& p);
    static std::vector<cv::Point> extractContourSegment(const std::vector<cv::Point>& contour, int i0, int i1);

    int detectEdgeType(PieceEdgeProfile* edgeProfile);

    PieceEdgeProfile* cvEdgeToPieceEdgeProfile(std::vector<cv::Point>& edge);
    void normalizeEdgeProfile(PieceEdgeProfile* edgeProfile);
    static inline float dist2(float x1, float y1, float x2, float y2);
    static inline float dist(float x1, float y1, float x2, float y2);
    inline void rotatePoint(float& x, float& y, float c, float s);


private:
    int findMinAreaReact(cv::Mat maskFilled, std::string outputDir, cv::Mat img);
    
};


#endif // PieceScan_H