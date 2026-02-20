#ifndef PIECEEDGEPROFILE_H
#define PIECEEDGEPROFILE_H
#include <vector>
#include "EdgePoint.h"
#include "EdgeMatch.h"

class PieceEdgeProfile
{
public:

    std::vector<EdgePoint*> edgePoints; // die tatsächlichen Punkte der Kante (optional, für Debug/Visualisierung)

    int edgeType; // 0 = gerade, 1 = weiblich, 2 = männlich

    std::vector<EdgeMatch*> matches; // mögliche Matches zu anderen Kanten (optional, für Debug/Visualisierung)
};

#endif // PIECEEDGEPROFILE_H
