#ifndef PIECEINFO_H
#define PIECEINFO_H
#include <QString>
#include "PieceEdgeProfile.h"


class PieceInfo
{
public:
    PieceInfo() {
        for(int i = 0; i < 4; i++){
            edgeProfile[i] = new PieceEdgeProfile();
            edgeProfileInverted[i] = new PieceEdgeProfile();
        }
    };

    int id = -1;
    QString name;
    QString imagePath;
    PieceEdgeProfile* edgeProfile[4] = {nullptr, nullptr, nullptr, nullptr}; // 0..3
    PieceEdgeProfile* edgeProfileInverted[4] = {nullptr, nullptr, nullptr, nullptr}; // optional: vorgefertigte Inversionen der Kantenprofile (für schnelleren Vergleich)
};

#endif // PIECEINFO_H