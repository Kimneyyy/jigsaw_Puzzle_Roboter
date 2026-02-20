#include "Board.h"
#include "Placement.h"
#include <iostream>
#include "pieceInfo.h"
#include "PieceEdgeProfile.h"
#include "EdgeMatch.h"
#include "AppController.h"
#include "Helpers.h"


Board::~Board()
{
    for(Placement* p : placements){
        delete p;
    }
}


void Board::addPiece(int pieceID, int x, int y, int rotation)
{
    Placement* p = new Placement();
    p->pieceID = pieceID;
    p->x = x;
    p->y = y;
    p->rotation = rotation;
    placements.push_back(p);
}

Placement* Board::getPlacement(int x, int y)
{
    for(Placement* p : placements){
        if(p->x == x && p->y == y){
            return p;
        }
    }
    return nullptr;
}


bestNextPlace* Board::getBestNextPlace(int pieceID, int edgeID, std::vector<PieceInfo*> pieces)
{
    double bestScore = -1;
    int bestPieceID = -1;
    int bestEdgeID = -1;
    int bestX = 0;
    int bestY = 0;
    int positionRelativeToNeighbor = 0; // 0=oben, 1=rechts, 2=unten, 3=links

    for(int p = 0; p < placements.size(); p++){
        Placement* sidePlacement[4] = {nullptr, nullptr, nullptr, nullptr};

        sidePlacement[0] = getPlacement(placements[p]->x, placements[p]->y - 1); // oben
        sidePlacement[1] = getPlacement(placements[p]->x + 1, placements[p]->y); // rechts
        sidePlacement[2] = getPlacement(placements[p]->x, placements[p]->y + 1); // unten
        sidePlacement[3] = getPlacement(placements[p]->x - 1, placements[p]->y); // links

        for(int s = 0; s < 4; s++){
            if(sidePlacement[s] != nullptr) continue; // wenn schon ein Teil da ist, überspringen

            int rotationSteps_pieceA = placements[p]->rotation / 90;
            int i_edgeA = Helpers::wrapIndex(s + rotationSteps_pieceA, 4);

            PieceEdgeProfile* edgeA = pieces[placements[p]->pieceID]->edgeProfile[i_edgeA];
            PieceEdgeProfile* edgeB = pieces[pieceID]->edgeProfile[edgeID];

            double score = appController->matchEdges(placements[p]->pieceID, i_edgeA, pieceID, edgeID);

            // get Rotation for Piece B: damit die Kante von B an die Kante von A passt, muss B um (s - edgeID) Schritte nach rechts rotiert werden

            if(score < bestScore || bestScore < 0){
                bestScore = score;
                bestPieceID = placements[p]->pieceID;
                bestEdgeID = (placements[p]->rotation / 90 + s) % 4;
                bestX = placements[p]->x + (s == 1 ? 1 : (s == 3 ? -1 : 0)); // rechts = +1, links = -1
                bestY = placements[p]->y + (s == 2 ? 1 : (s == 0 ? -1 : 0)); // unten = +1, oben = -1
                positionRelativeToNeighbor = s;
            }
        }
    }

    if(bestScore < 0) return nullptr; // kein Platz gefunden

    bestNextPlace* bnp = new bestNextPlace();
    bnp->pieceID = bestPieceID;
    bnp->edgeID = bestEdgeID;
    bnp->matchScore = bestScore;
    bnp->x = bestX;
    bnp->y = bestY;
    bnp->positionRelativeToNeighbor = positionRelativeToNeighbor;
    return bnp;
}


Placement* Board::getPieceByID(int pieceID)
{
    for(Placement* p : placements){
        if(p->pieceID == pieceID){
            return p;
        }
    }
    return nullptr;
}


void Board::normalizeBoardToZero()
{
    if(placements.empty()) return;

    int minX = placements[0]->x;
    int minY = placements[0]->y;

    for(Placement* p : placements){
        if(p->x < minX) minX = p->x;
        if(p->y < minY) minY = p->y;
    }

    for(Placement* p : placements){
        p->x -= minX; // da minX negativ ist, verschiebt das alle Teile nach rechts
        p->y -= minY;
    }

}