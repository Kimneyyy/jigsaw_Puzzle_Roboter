#include "pieceScan.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include "pieceInfo.h"
#include "PieceEdgeProfile.h"
#include "EdgePoint.h"
#include "SquareFinder.h"
#include "Helpers.h"


PieceScan::PieceScan(/* args */)
{
}


PieceScan::~PieceScan()
{
}


int PieceScan::scanPiece(PieceInfo* piece, const std::string& imagePath, const std::string& outputDir)
{

    cv::Mat img = cv::imread(imagePath);
    if (img.empty()) {
        std::cerr << "Bild nicht gefunden.\n";
        return 1;
    }

    // 1) HSV (robust bei Helligkeit) Farben Ändern. Dunkel = Gruen, Hell = Rot
    cv::Mat hsv;
    cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);
    saveDebugImage(outputDir, "hsv.png", hsv);


    // 2) Alles was nicht weiß ist, ist ein Teil des Stücks. Weiß = S niedrig, Dunkel = S hoch
    cv::Mat maskRaw;
    // nicht-weiß: S hoch genug
    cv::inRange(hsv, cv::Scalar(0, 40, 0), cv::Scalar(179, 255, 255), maskRaw);
    saveDebugImage(outputDir, "mask_raw.png", maskRaw);

    // 3) Konturen finden
    std::vector<std::vector<cv::Point>> contours = findContours(maskRaw);
    std:: cout << "Anzahl Konturen: " << contours.size() << std::endl;
    // 4) Größte Kontur auswählen
    std::vector<cv::Point> biggestContour = findBiggestContour(contours);
    saveDebugImage(outputDir, "biggest_contour.png", img, biggestContour);


    // 4) “perfekte” gefüllte Maske aus der Kontur bauen (keine Löcher möglich)
    cv::Mat maskFilled = cv::Mat::zeros(maskRaw.size(), CV_8UC1);
    cv::drawContours(maskFilled, std::vector<std::vector<cv::Point>>{biggestContour}, -1, 255, cv::FILLED);
    saveDebugImage(outputDir, "mask_filled_from_contour.png", maskFilled);


    int radius = findMinAreaReact(maskFilled, outputDir, img);
    std::cout << "Best radius: " << radius << "\n";

    // 2) Erosion -> Noppen "abknipsen"
    int erodeRadiusPx = radius;  // 55
    int k = 2 * erodeRadiusPx + 1;
    cv::Mat se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(k, k));
    cv::Mat core;
    cv::erode(maskFilled, core, se, cv::Point(-1,-1), 1);
    saveDebugImage(outputDir, "eroded_mask.png", core);

    // 3) Konturen finden ohne Noppen
    std::vector<std::vector<cv::Point>> contours_no_nops = findContours(core);
    std:: cout << "Anzahl Konturen ohne Noppen: " << contours_no_nops.size() << std::endl;
    // 4) Größte Kontur auswählen
    std::vector<cv::Point> biggestContour_no_nops = findBiggestContour(contours_no_nops);
    saveDebugImage(outputDir, "biggest_contour_no_nops.png", img, biggestContour_no_nops);

    
    // 6) Kontur stark vereinfachen und Glaetten, damit die Kontur nicht so viele Punkte hat
    double per = cv::arcLength(biggestContour_no_nops, true);
    double eps = 0.012 * per;                 // 2% ist guter Start
    std::vector<cv::Point> poly;
    cv::approxPolyDP(biggestContour_no_nops, poly, eps, true);
    saveDebugImage(outputDir, "approximated_contour.png", img, poly);

    cv::RotatedRect rr = cv::minAreaRect(poly);
    cv::Point2f box[4];
    rr.points(box);
    saveDebugImage(outputDir, "min_area_rect.png", img, std::vector<cv::Point>(box, box + 4));


    auto nearestOnContour = [&](const cv::Point2f& p){
        int bestI = -1; float bestD = 1e30f;
        for (int i=0;i<(int)poly.size();++i){
            cv::Point2f q(poly[i].x, poly[i].y);
            float d = (q.x-p.x)*(q.x-p.x) + (q.y-p.y)*(q.y-p.y);
            if (d < bestD){ bestD = d; bestI = i; }
        }
        return poly[bestI];
    };

    std::vector<cv::Point> corners;
    for(int i=0;i<4;++i) corners.push_back(nearestOnContour(box[i]));
    saveDebugImage(outputDir, "corners_innen.png", img, corners);

    // find outer corners by extending the lines from the center to the corners
    cv::Point2f center = rr.center;
    std::vector<cv::Point> outerCorners;
    for (int i=0;i<4;i++) {
        cv::Point2f dir = cv::Point2f(corners[i].x, corners[i].y) - center;
        float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
        if (len > 1e-5) {
            dir *= (1.0f / len);
            cv::Point2f outer = cv::Point2f(corners[i].x, corners[i].y) + dir * 100.0f; // 100px weiter raus
            outerCorners.push_back(cv::Point((int)std::round(outer.x), (int)std::round(outer.y)));
        } else {
            outerCorners.push_back(corners[i]);
        }
    }
    saveDebugImage(outputDir, "corners_außen.png", img, outerCorners);

    // find reals approximate corners
    std::vector<cv::Point2f> trueApproximateCorners;
    for (int i=0;i<4;i++) {
        cv::Point2f c = findFirstMaskHitAlongLine(maskFilled, corners[i], outerCorners[i]);
        trueApproximateCorners.push_back(c);
    }
    saveDebugImage(outputDir, "true_corners_approximate.png", img, std::vector<cv::Point>(trueApproximateCorners.begin(), trueApproximateCorners.end()));



    // Find real corners by looking for curvature peaks along the contour in the vicinity of the approximated corners
    std::vector<cv::Point> trueCorners;
    for (int i=0;i<4;i++) {
        int iDavor = i-1; if (iDavor < 0) iDavor = 3;
        int iNachfolger = i+1; if (iNachfolger > 3) iNachfolger = 0;
        
        cv::Point approxCorner = trueApproximateCorners[i];
        cv::Point trueCorner = findNextCornerAlongLine(approxCorner, trueApproximateCorners[iDavor], trueApproximateCorners[iNachfolger], biggestContour); // nochmal mit Richtung
        //trueCorner = findNextCornerAlongLine(trueCorner, biggestContour);
        trueCorners.push_back(trueCorner);
    }
    // Corners so sortieren, dass oben links corner[0] ist, dann im Uhrzeigersinn weiter
    trueCorners = sortCorners(trueCorners);
    saveDebugImage(outputDir, "true_corners.png", img, trueCorners);

    
    // Extract edge profiles by finding the contour segments between the corners
    // 2) Indizes der Ecken auf der Kontur holen
    std::vector<int> cornerIdx(4);
    for (int i=0;i<4;i++) cornerIdx[i] = nearestContourIndex(biggestContour, trueCorners[i]);

    // 3) 4 Kontur-Segmente extrahieren (zwischen Ecke i und i+1)
    std::vector<std::vector<cv::Point>> edges(4);
    edges[0] = extractContourSegment(biggestContour, cornerIdx[0], cornerIdx[1]);
    edges[1] = extractContourSegment(biggestContour, cornerIdx[1], cornerIdx[2]);
    edges[2] = extractContourSegment(biggestContour, cornerIdx[2], cornerIdx[3]);
    edges[3] = extractContourSegment(biggestContour, cornerIdx[3], cornerIdx[0]); // wrap

    // Debug optional
    for(int i=0;i<4;i++){
        saveDebugImage(outputDir, "edge"+std::to_string(i)+".png", img, edges[i]);
    }

    // 4) Kantenprofile in PieceEdgeProfile umwandeln
    for(int i=0;i<4;i++){
        piece->edgeProfile[i] = cvEdgeToPieceEdgeProfile(edges[i]);
    }

    // Kanten auf x-Achse normalisieren (für Vergleichbarkeit)
    for(int i=0;i<4;i++){
        normalizeEdgeProfile(piece->edgeProfile[i]);
    }


    for(int i=0;i<4;i++)
    {
        piece->edgeProfile[i]->edgeType = detectEdgeType(piece->edgeProfile[i]);
        piece->edgeProfileInverted[i]->edgeType = piece->edgeProfile[i]->edgeType;
    }

    return 0;

}


int PieceScan::findMinAreaReact(cv::Mat maskFilled, std::string outputDir, cv::Mat img)
{
    auto clamp01 = [](double v){ return std::max(0.0, std::min(1.0, v)); };

    double bestScore = -1.0;
    int bestRadius = -1;
    cv::Mat bestCore;
    cv::RotatedRect bestRR;
    std::vector<cv::Point> bestContourNoNops;

    for (int radius = 50; radius <= 65; radius += 5)
    {
        // 2) Erosion -> Noppen "abknipsen"
        int erodeRadiusPx = radius;  // 55
        int k = 2 * erodeRadiusPx + 1;
        cv::Mat se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(k, k));
        cv::Mat core;
        cv::erode(maskFilled, core, se, cv::Point(-1,-1), 1);
        //saveDebugImage(outputDir, "eroded_mask.png", core);

        // 3) Konturen finden ohne Noppen
        std::vector<std::vector<cv::Point>> contours_no_nops = findContours(core);
        // 4) Größte Kontur auswählen
        std::vector<cv::Point> biggestContour_no_nops = findBiggestContour(contours_no_nops);
        //saveDebugImage(outputDir, "biggest_contour_no_nops.png", img, biggestContour_no_nops);

        
        // 6) Kontur stark vereinfachen und Glaetten, damit die Kontur nicht so viele Punkte hat
        double per = cv::arcLength(biggestContour_no_nops, true);
        double eps = 0.012 * per;                 // 2% ist guter Start
        std::vector<cv::Point> poly;
        cv::approxPolyDP(biggestContour_no_nops, poly, eps, true);
        //saveDebugImage(outputDir, "approximated_contour.png", img, poly);

        cv::RotatedRect rr = cv::minAreaRect(poly);
        cv::Point2f box[4];
        rr.points(box);
        //saveDebugImage(outputDir, "min_area_rect.png", img, std::vector<cv::Point>(box, box + 4));
        double minAreaReactScore = SquareFinder::rectangleScore01(std::vector<cv::Point>(box, box + 4));


        auto nearestOnContour = [&](const cv::Point2f& p){
            int bestI = -1; float bestD = 1e30f;
            for (int i=0;i<(int)poly.size();++i){
                cv::Point2f q(poly[i].x, poly[i].y);
                float d = (q.x-p.x)*(q.x-p.x) + (q.y-p.y)*(q.y-p.y);
                if (d < bestD){ bestD = d; bestI = i; }
            }
            return poly[bestI];
        };

        std::vector<cv::Point> corners;
        for(int i=0;i<4;++i) corners.push_back(nearestOnContour(box[i]));
        QString debugName = QString("corners_innen_radius_%1.png").arg(radius);
        saveDebugImage(outputDir, debugName.toStdString(), img, corners);

        // find square score: wie sehr ähnelt die Kontur einem Rechteck? (0..1, 1=perfektes Rechteck)
        double score = SquareFinder::rectangleScore01(corners);
        if (score > bestScore) {
            bestScore = score;
            bestRadius = radius;
            bestCore = core;
            bestRR = rr;
            bestContourNoNops = biggestContour_no_nops;
        }
    }

    // Ergebnis prüfen
    if (bestRadius < 0) {
        std::cerr << "Kein sinnvoller Core-Kandidat gefunden.\n";
        return 1;
    }

    return bestRadius;
}


void PieceScan::saveDebugImage(std::string path, std::string name, cv::Mat img)
{
    std::string outputPath = path + "/" + name;
    cv::imwrite(outputPath, img);
}

void PieceScan::saveDebugImage(std::string path, std::string name, cv::Mat img, std::vector<cv::Point> contour)
{
    std::vector<std::vector<cv::Point>> contours;
    contours.push_back(contour);

    cv::Mat debugImg;
    img.copyTo(debugImg);
    cv::drawContours(debugImg, contours, 0, cv::Scalar(0, 255, 0), 2);

    saveDebugImage(path, name, debugImg);
}

void PieceScan::saveDebugCSV_EdgeProfile(PieceEdgeProfile* edgeProfile)
{
    std::string csvPath = "/home/kimi-sickinger/Programmieren/qt/piecePilot_v2/src/out/edgeProfile.csv";
    int copyIndex = 1;
    while (std::ifstream(csvPath).good()) {
        csvPath = "/home/kimi-sickinger/Programmieren/qt/piecePilot_v2/src/out/edgeProfile_" + std::to_string(copyIndex) + ".csv";
        copyIndex++;
    }

    std::ofstream csvFile(csvPath);
    csvFile << "x,y\n";
    for(size_t i=0; i<edgeProfile->edgePoints.size(); i++){
        csvFile << edgeProfile->edgePoints[i]->x << "," << edgeProfile->edgePoints[i]->y << "\n";
    }
    csvFile.close();
}

std::vector<std::vector<cv::Point>> PieceScan::findContours(const cv::Mat& mask)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    return contours;
}

std::vector<cv::Point> PieceScan::findBiggestContour(const std::vector<std::vector<cv::Point>>& contours)
{
    double maxArea = 0;
    int maxAreaIdx = -1;
    for (size_t i = 0; i < contours.size(); i++) {
        double area = cv::contourArea(contours[i]);
        if (area > maxArea) {
            maxArea = area;
            maxAreaIdx = i;
        }
    }

    if (maxAreaIdx >= 0) {
        return contours[maxAreaIdx];
    } else {
        return std::vector<cv::Point>(); // leere Kontur zurückgeben, wenn keine Konturen gefunden wurden
    }
}

cv::Rect PieceScan::findBoundingBox(const cv::Mat& img, std::vector<cv::Point> contour, std::string outputDir)
{
    
    std::vector<std::vector<cv::Point>> contours;
    contours.push_back(contour);
    
    // Debug: Kontur mit Bounding Box zeichnen
    if(outputDir != "") {
        cv::Mat debugImg;
        img.copyTo(debugImg);
        cv::drawContours(debugImg, contours, 0, cv::Scalar(0, 255, 0), 2);
        cv::Rect boundingBox = cv::boundingRect(contour);
        cv::rectangle(debugImg, boundingBox, cv::Scalar(255, 0, 0), 2);
        saveDebugImage(outputDir, "contour_with_bbox.png", debugImg);
    }

    // Bounding Box der größten Kontur zurückgeben
    return cv::boundingRect(contour);
}


cv::Point2f PieceScan::findFirstMaskHitAlongLine(const cv::Mat& mask, cv::Point2f p0, cv::Point2f p1)
{
    // p0 -> p1 in N Schritten ablaufen
    cv::Point2f d = p1 - p0;
    float len = std::sqrt(d.x*d.x + d.y*d.y);
    if (len < 1.0f) return p0;
    d *= (1.0f / len);

    // Step = 0.5px für stabilen Rand
    float step = 0.5f;
    int steps = (int)(len / step);

    bool wasInside = false;
    cv::Point2f lastInside = p0;

    for (int i=0; i<=steps; ++i) {
        cv::Point2f p = p0 + d * (i * step);
        int x = (int)std::round(p.x);
        int y = (int)std::round(p.y);
        if (x < 0 || y < 0 || x >= mask.cols || y >= mask.rows) continue;

        bool inside = (mask.at<uchar>(y, x) > 0);

        // wenn du von innen nach außen gehst: suche Übergang 255 -> 0
        if (inside) {
            wasInside = true;
            lastInside = p;
        } else {
            if (wasInside) {
                return lastInside; // Randpunkt
            }
        }
    }
    return lastInside;
}


cv::Point PieceScan::findNextCornerAlongLine(cv::Point p0, std::vector<cv::Point> contour)
{
    // Findet die "echte" Ecke in der Nähe von p0, indem entlang der Kontur
    // im Umkreis ein Winkel-/Krümmungs-Peak gesucht wird.

    if (contour.empty()) return p0;

    auto wrapIndex = [&](int i, int n) -> int {
        i %= n;
        if (i < 0) i += n;
        return i;
    };

    auto nearestContourIndex = [&](const cv::Point& p) -> int {
        int bestI = 0;
        long long bestD = (1LL<<62);
        for (int i = 0; i < (int)contour.size(); ++i) {
            long long dx = (long long)contour[i].x - p.x;
            long long dy = (long long)contour[i].y - p.y;
            long long d  = dx*dx + dy*dy;
            if (d < bestD) { bestD = d; bestI = i; }
        }
        return bestI;
    };

    auto angleAt = [&](int i, int s) -> float {
        int n = (int)contour.size();
        cv::Point2f P((float)contour[wrapIndex(i, n)].x,     (float)contour[wrapIndex(i, n)].y);
        cv::Point2f A((float)contour[wrapIndex(i - s, n)].x, (float)contour[wrapIndex(i - s, n)].y);
        cv::Point2f B((float)contour[wrapIndex(i + s, n)].x, (float)contour[wrapIndex(i + s, n)].y);

        cv::Point2f v1 = A - P;
        cv::Point2f v2 = B - P;

        float n1 = std::sqrt(v1.x*v1.x + v1.y*v1.y);
        float n2 = std::sqrt(v2.x*v2.x + v2.y*v2.y);
        if (n1 < 1e-6f || n2 < 1e-6f) return 3.1415926f;

        float dot = (v1.x*v2.x + v1.y*v2.y) / (n1*n2);
        dot = std::max(-1.0f, std::min(1.0f, dot));
        return std::acos(dot); // 0..pi
    };

    // ---- Parameter (bei Bedarf anpassen) ----
    // Suche um p0 entlang der Kontur:
    int n = (int)contour.size();
    int windowPts = std::max(20, (int)(0.017f * n));   // ~2% der Kontur
    int stepPts   = std::max(10, (int)(0.004f * n));  // ~0.4% der Kontur
    // Optional: Caps, damit es nicht zu groß wird
    windowPts = std::min(windowPts, 250);
    stepPts   = std::min(stepPts, 60);

    int i0 = nearestContourIndex(p0);

    int bestI = i0;
    float bestScore = -1e30f;

    for (int di = -windowPts; di <= windowPts; ++di) {
        int i = wrapIndex(i0 + di, n);
        float ang = angleAt(i, stepPts);

        // kleiner Winkel => stärkere Ecke => höherer Score
        float score = 3.1415926f - ang;

        if (score > bestScore) {
            bestScore = score;
            bestI = i;
        }
    }

    return contour[bestI];
}


cv::Point PieceScan::findNextCornerAlongLine(cv::Point pMitte, cv::Point pDavor, cv::Point pNachfolger, std::vector<cv::Point> contour)
{

    int firstI = nearestContourIndex(contour, pMitte);

    // finde Punkt, der am weitesten von beiden Punk rten entfernt liegt
    int bestI = firstI;
    long long bestD = 0;
    int startI = Helpers::wrapIndex(firstI - 40, (int)contour.size()); // 40 Punkte
    int endI   = Helpers::wrapIndex(firstI + 40, (int)contour.size()); // 80 Punkte Fenster

    for(int i = startI; i != endI; i = Helpers::wrapIndex(i + 1, (int)contour.size()))
    {
        double dist1 = Helpers::getDistance(contour[i], pDavor);
        double dist2 = Helpers::getDistance(contour[i], pNachfolger);

        double score = dist1 + dist2; // Je weiter weg von beiden, desto besser
        if(score > bestD)
        {
            bestD = score;
            bestI = i;
        }
    }

    return contour[bestI];
}
 

std::vector<cv::Point> PieceScan::sortCorners(std::vector<cv::Point> corners)
{
    // Sortiere die Ecken so, dass die obere linke Ecke an erster Stelle steht und dann im Uhrzeigersinn weitergeht
    if (corners.size() != 4) return corners; // Fehler: Es sollten genau 4 Ecken sein

    // Finde die obere linke Ecke (kleinste x+y)
    int bestI = 0;
    int bestSum = corners[0].x + corners[0].y;
    for (int i = 1; i < 4; i++) {
        int sum = corners[i].x + corners[i].y;
        if (sum < bestSum) {
            bestSum = sum;
            bestI = i;
        }
    }

    // Sortiere im Uhrzeigersinn beginnend mit der gefundenen Ecke
    std::vector<cv::Point> sorted(4);
    for (int i = 0; i < 4; i++) {
        sorted[i] = corners[(bestI + i) % 4];
    }
    return sorted;
}




int PieceScan::nearestContourIndex(const std::vector<cv::Point>& contour, const cv::Point& p)
{
    int bestI = 0;
    long long bestD = (1LL<<62);
    for (int i=0;i<(int)contour.size();++i){
        long long dx = (long long)contour[i].x - p.x;
        long long dy = (long long)contour[i].y - p.y;
        long long d  = dx*dx + dy*dy;
        if (d < bestD){ bestD = d; bestI = i; }
    }
    return bestI;
}

std::vector<cv::Point> PieceScan::extractContourSegment(
    const std::vector<cv::Point>& contour,
    int i0,
    int i1)
{
    std::vector<cv::Point> seg;
    int n = (int)contour.size();
    if (n == 0) return seg;

    i0 = (i0 % n + n) % n;
    i1 = (i1 % n + n) % n;

    // gehe rueckwerts i0 -> i1 mit Wrap
    int i = i0;
    while (true) {
        seg.push_back(contour[i]);
        if (i == i1) break;
        i = (i - 1 + n) % n;
    }
    return seg;
}


PieceEdgeProfile* PieceScan::cvEdgeToPieceEdgeProfile(std::vector<cv::Point>& edge)
{
    PieceEdgeProfile* edgeProfile = new PieceEdgeProfile();

    for(int i = 0; i < edge.size(); i++) {
        EdgePoint* ep = new EdgePoint();
        ep->x = (float)edge[i].x;
        ep->y = (float)edge[i].y;
        edgeProfile->edgePoints.push_back(ep);
    }

    return edgeProfile;
}



void PieceScan::normalizeEdgeProfile(PieceEdgeProfile* edgeProfile)
{
    //saveDebugCSV_EdgeProfile(edgeProfile);


    if (!edgeProfile) return;
    auto& ptsPtr = edgeProfile->edgePoints;

    if (ptsPtr.size() < 2) return;

    // ===== 1) Kopie in lokale Float-Punkte (und Null/NaN abfangen) =====
    struct P { float x,y; };
    std::vector<P> pts;
    pts.reserve(ptsPtr.size());

    for (auto* ep : ptsPtr)
    {
        if (!ep) continue;
        if (!std::isfinite(ep->x) || !std::isfinite(ep->y)) continue;
        pts.push_back({ep->x, ep->y});
    }
    if (pts.size() < 2) return;

    // Optional: doppelte aufeinanderfolgende Punkte entfernen
    {
        std::vector<P> cleaned;
        cleaned.reserve(pts.size());
        cleaned.push_back(pts[0]);
        for (size_t i=1; i<pts.size(); ++i)
        {
            if (dist2(cleaned.back().x, cleaned.back().y, pts[i].x, pts[i].y) > 1e-10f)
                cleaned.push_back(pts[i]);
        }
        pts.swap(cleaned);
        if (pts.size() < 2) return;
    }

    // ===== 2) Bogenlänge + Resampling auf feste Anzahl =====
    const int N = 256; // Standard. Kannst du später tunen.
    std::vector<float> s(pts.size(), 0.0f);
    for (size_t i=1; i<pts.size(); ++i)
        s[i] = s[i-1] + dist(pts[i-1].x, pts[i-1].y, pts[i].x, pts[i].y);

    float totalLen = s.back();
    if (totalLen < 1e-6f) return;

    std::vector<P> res;
    res.resize(N);

    // Zielabstände
    for (int k=0; k<N; ++k)
    {
        float target = (totalLen * k) / (N-1);

        // finde Segment [j-1, j]
        auto it = std::lower_bound(s.begin(), s.end(), target);
        size_t j = (it == s.end()) ? (s.size()-1) : (size_t)(it - s.begin());

        if (j == 0) { res[k] = pts[0]; continue; }
        size_t i0 = j - 1;
        size_t i1 = j;

        float s0 = s[i0];
        float s1 = s[i1];
        float t = (s1 > s0) ? (target - s0) / (s1 - s0) : 0.0f;

        res[k].x = pts[i0].x + t * (pts[i1].x - pts[i0].x);
        res[k].y = pts[i0].y + t * (pts[i1].y - pts[i0].y);
    }

    // ===== 3) Translation: Startpunkt auf (0,0) =====
    {
        float x0 = res[0].x;
        float y0 = res[0].y;
        for (auto& p : res) { p.x -= x0; p.y -= y0; }
    }

    // ===== 4) Rotation: Endpunkt auf +X Achse =====
    {
        float ex = res.back().x;
        float ey = res.back().y;

        float ang = std::atan2(ey, ex);
        float c = std::cos(-ang);
        float s_ = std::sin(-ang);

        for (auto& p : res) rotatePoint(p.x, p.y, c, s_);
    }

    // ===== 5) Richtung konsistent: Endpunkt muss rechts liegen (x >= 0) =====
    // Nach Rotation sollte das schon so sein, aber falls numerisch oder Start/End vertauscht:
    if (res.back().x < 0.0f)
    {
        std::reverse(res.begin(), res.end());

        // wieder Startpunkt auf 0
        float x0 = res[0].x;
        float y0 = res[0].y;
        for (auto& p : res) { p.x -= x0; p.y -= y0; }

        // wieder rotieren
        float ex = res.back().x;
        float ey = res.back().y;
        float ang = std::atan2(ey, ex);
        float c = std::cos(-ang);
        float s_ = std::sin(-ang);
        for (auto& p : res) rotatePoint(p.x, p.y, c, s_);
    }

    // ===== 6) Scale: Länge = 1 =====
    {
        float L = std::sqrt(res.back().x*res.back().x + res.back().y*res.back().y);
        if (L < 1e-6f) return;
        float invL = 1.0f / L;
        for (auto& p : res) { p.x *= invL; p.y *= invL; }
    }

    // ===== 7) Ergebnis zurückschreiben (in deine EdgePoint-Objekte) =====
    // Einfach: alte Punkte löschen und neue N Punkte erzeugen.
    for (auto* ep : ptsPtr) delete ep;
    ptsPtr.clear();
    ptsPtr.reserve(N);

    for (int i=0; i<N; ++i)
    {
        EdgePoint* ep = new EdgePoint();
        ep->x = res[i].x;
        ep->y = res[i].y;
        ptsPtr.push_back(ep);
    }


    // Save Debug csv after normalization
    //saveDebugCSV_EdgeProfile(edgeProfile);
}



inline float PieceScan::dist2(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx*dx + dy*dy;
}

inline float PieceScan::dist(float x1, float y1, float x2, float y2)
{
    return std::sqrt(dist2(x1,y1,x2,y2));
}

inline void PieceScan::rotatePoint(float& x, float& y, float c, float s)
{
    float nx = c*x - s*y;
    float ny = s*x + c*y;
    x = nx; y = ny;
}



int PieceScan::detectEdgeType(PieceEdgeProfile* edgeProfile)
{
    // Wenn der Durchnitt der y Werte negativ ist, dann ist es wahrscheinlich eine männliche Kante (nach unten), wenn positiv, dann weiblich (nach oben), und wenn nahe 0, dann gerade.
    float sumY = 0.0f;
    for(int i = 0; i < edgeProfile->edgePoints.size(); i++) {
        sumY += edgeProfile->edgePoints[i]->y;
    }
    float avgY = sumY / edgeProfile->edgePoints.size();


    bool peakFound = false;
    // Wenn kein Peak von über 10% des Durchschnitts vorhanden ist, dann ist es wahrscheinlich eine gerade Kante. Das ist natürlich sehr grob, aber für den Anfang könnte es reichen.
    for(int i = 0; i < edgeProfile->edgePoints.size(); i++)
    {
        if(std::abs(edgeProfile->edgePoints[i]->y) > 0.1f * std::abs(avgY)) {
            peakFound = true;
            break;
        }
    }

    if (!peakFound) {
        return 0; // gerade
    }

    if(avgY < 0) return 2; // männlich (nach unten)
    if(avgY > 0) return 1; // weiblich (nach oben)

    return 0; // gerade
}