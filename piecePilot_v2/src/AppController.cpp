#include "AppController.h"
#include <cmath>
#include <algorithm>
#include "pieceScan.h"
#include "pieceInfo.h"
#include <iostream>
#include <QDir>
#include "EdgeMatch.h"
#include "Board.h"
#include "Placement.h"
#include "Helpers.h"

AppController::AppController()
    : QObject(nullptr)
{
    this->pieceScanner = new PieceScan();
    this->board = new Board(this);

    QString basePath = "/home/kimi-sickinger/Programmieren/qt/piecePilot_v2/src/img/";
    // get all png files in basePath
    QDir dir(basePath);
    QStringList filters;
    filters << "*.jpeg" << "*.png" << "*.bmp";
    QStringList paths = dir.entryList(filters, QDir::Files);
    for(const QString& path : paths){
        this->scanPiece((basePath + path).toStdString());
    }


    std::cout << "Computing Edge Matches ..." << std::endl;
    computeAllEdgeMatches();
    
    sortPieceToBoard();
}


void AppController::scanPiece(const std::string& imagePath)
{
    PieceInfo* piece = new PieceInfo();
    piece->imagePath = QString::fromStdString(imagePath);
    piece->id = (int)pieceInfo.size();
    piece->name = QString("Teil_%1").arg(piece->id);
    std::cout << "Scanning piece: " << piece->name.toStdString() << std::endl;
    std::string outputDir = "/home/kimi-sickinger/Programmieren/qt/piecePilot_v2/src/out/" + piece->name.toStdString() + "/";
    QDir().mkpath(QString::fromStdString(outputDir)); // create output directory if it doesn't exist
    this->pieceScanner->scanPiece(piece, imagePath, outputDir);
    /* auto r = this->pieceScanner->scanFromImage(imagePath, "/home/kimi-sickinger/Programmieren/qt/piecePilot/src/out/", piece);
    if (!r.ok) {
        std::cerr << r.error << "\n";
        //return 1;
    } */

    // Invert die Kantenprofile und speichere sie in piece->edgeProfileInverted, damit wir sie später schneller vergleichen können (ohne jedes Mal neu invertieren zu müssen)
    for(int i = 0; i < 4; i++){
        piece->edgeProfileInverted[i] = invertEdgeProfile(piece->edgeProfile[i]);
    }


    pieceInfo.push_back(piece);
    

}

std::vector<PieceInfo*> AppController::getPieces()
{
    return pieceInfo;
}

PieceEdgeProfile* AppController::getEdgeProfile(int pieceID, int edgeID)
{
    if(pieceID < 0 || pieceID >= (int)pieceInfo.size()) return nullptr;
    PieceInfo* piece = pieceInfo[pieceID];
    if(edgeID < 0 || edgeID >= 4) return nullptr;
    return piece->edgeProfile[edgeID];
}

PieceEdgeProfile* AppController::getInvertedEdgeProfile(int pieceID, int edgeID)
{
    if(pieceID < 0 || pieceID >= (int)pieceInfo.size()) return nullptr;
    PieceInfo* piece = pieceInfo[pieceID];
    if(edgeID < 0 || edgeID >= 4) return nullptr;
    return piece->edgeProfileInverted[edgeID];
}


double AppController::matchEdges(int pieceA_id, int edgeA_id, int pieceB_id, int edgeB_id)
{
    PieceEdgeProfile* profA = getEdgeProfile(pieceA_id, edgeA_id);
    PieceEdgeProfile* profB = getInvertedEdgeProfile(pieceB_id, edgeB_id); // Verwende die vorberechnete invertierte Kante
    if(!profA || !profB) return std::numeric_limits<double>::infinity();

    double score = 0.0;

    int minPoints = std::min(profA->edgePoints.size(), profB->edgePoints.size());
    for(int i = 0; i < minPoints; i++){
        double dist = std::sqrt(std::pow(profA->edgePoints[i]->x - profB->edgePoints[i]->x, 2) + std::pow(profA->edgePoints[i]->y - profB->edgePoints[i]->y, 2));
        score += dist;
    }


    return score;
}


PieceEdgeProfile* AppController::invertEdgeProfile(PieceEdgeProfile* prof)
{

    if (!prof) return nullptr;
    PieceEdgeProfile* inverted_x = new PieceEdgeProfile();

    int n = (int)prof->edgePoints.size();
    for(int i = 0; i < n; i++){
        EdgePoint* ep = new EdgePoint();
        ep->x = - prof->edgePoints[n - 1 - i]->x + prof->edgePoints[n - 1]->x;
        ep->y = - prof->edgePoints[n - 1 - i]->y;
        inverted_x->edgePoints.push_back(ep);
    }

    return inverted_x;
}


void AppController::computeAllEdgeMatches()
{
    for(int pA = 0; pA < pieceInfo.size(); pA++){
        PieceInfo* pieceA = pieceInfo[pA];
        for(int eA = 0; eA < 4; eA++){
            PieceEdgeProfile* edgeA = pieceA->edgeProfile[eA];
            if(edgeA->edgeType == 0) continue; // gerade Kanten überspringen, da sie wahrscheinlich nicht so gut passen
            for(int pB = 0; pB < pieceInfo.size(); pB++){
                if(pA == pB) continue; // gleiche Teile überspringen
                PieceInfo* pieceB = pieceInfo[pB];
                for(int eB = 0; eB < 4; eB++){
                    PieceEdgeProfile* edgeB = pieceB->edgeProfile[eB];
                    if(edgeB->edgeType == 0) continue; // gerade Kanten überspringen, da sie wahrscheinlich nicht so gut passen
                    if(edgeA->edgeType == edgeB->edgeType) continue; // gleiche Kantentypen überspringen, da sie wahrscheinlich nicht so gut passen (z.B. weiblich-weiblich oder männlich-männlich
                    
                    double score = matchEdges(pA, eA, pB, eB);
                    EdgeMatch* match = new EdgeMatch();
                    match->pieceID = pB;
                    match->edgeID = eB;
                    match->score = (float)score;


                    // find spot in edgeA->matches to insert this match sorted by score
                    if(edgeA->matches.size() == 0){
                        edgeA->matches.push_back(match);
                    }
                    else{
                        for(int i = 0; i < edgeA->matches.size(); i++){
                            if(score < edgeA->matches[i]->score){
                                edgeA->matches.insert(edgeA->matches.begin() + i, match);
                                break;
                            }
                            if(i == edgeA->matches.size() - 1){
                                edgeA->matches.push_back(match);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

}

PieceInfo* AppController::getBestPieceToStartBoard()
{
    PieceInfo* bestPiece = nullptr;
    double bestScore = std::numeric_limits<double>::infinity();

    for(int i = 0; i < pieceInfo.size(); i++){
        PieceInfo* piece = pieceInfo[i];
        
        for(int e = 0; e < 4; e++){
            if(piece->edgeProfile[e]->matches.size() == 0) continue; // wenn keine Matches, überspringen
            double score = piece->edgeProfile[e]->matches[0]->score; // bester Match
            if(score < bestScore){
                bestScore = score;
                bestPiece = piece;
            }
        }
    }

    return bestPiece;

}


void AppController::sortPieceToBoard()
{
    //PieceInfo* startPiece = getBestPieceToStartBoard();

    board->addPiece(0, 0, 0, 0); // erstes Teil in die Mitte legen, Rotation 0


    while(board->getPlacements().size() < pieceInfo.size()){
        double bestScore = std::numeric_limits<double>::infinity();
        int bestX = 0;
        int bestY = 0;
        int bestPositionRelativeToNeighbor = 0;
        int bestPieceID = -1;
        int bestEdgeID = -1;

        for(int i = 0; i < pieceInfo.size(); i++){
            PieceInfo* piece = pieceInfo[i];

            if(board->getPieceByID(piece->id) != nullptr){
                continue; // Teil liegt schon auf dem Board, überspringen
            }

            for(int e=0; e < 4; e++){
                bestNextPlace* bnp = board->getBestNextPlace(piece->id, e, pieceInfo);
                if(!bnp) continue; // kein Platz gefunden, überspringen

                if(bnp->matchScore < bestScore){
                    bestScore = bnp->matchScore;
                    bestPieceID = piece->id;
                    bestX = bnp->x;
                    bestY = bnp->y;
                    bestPositionRelativeToNeighbor = bnp->positionRelativeToNeighbor;
                    bestEdgeID = e;
                }
            }

            //board->addPiece(piece->id, x, y, rotation);
        }

        // Hier jetzt berechnung, welche Rotation das neue Teil haben muss, damit die Kante an die Nachbarkante passt
        int bestRotation = 0;
        if(bestPositionRelativeToNeighbor == 0){ // oben
            // wenn das neue Teil oben liegt, muss die Kante e des neuen Teils an
            // die Kante (bestEdgeID) des Nachbarteils passen. Damit das passt, muss das neue Teil um (bestEdgeID - e) Schritte nach rechts rotiert werden
            bestRotation = Helpers::wrapIndex(bestEdgeID -2, 4) * 90; // -2
        }
        else if(bestPositionRelativeToNeighbor == 1){ // rechts
            bestRotation = Helpers::wrapIndex(bestEdgeID - 3, 4) * 90;  // -1
        }
        else if(bestPositionRelativeToNeighbor == 2){ // unten
            bestRotation = Helpers::wrapIndex(bestEdgeID - 0, 4) * 90; // 0
        }
        else if(bestPositionRelativeToNeighbor == 3){ // links
            bestRotation = Helpers::wrapIndex(bestEdgeID - 1, 4) * 90;  // -3
        }

        board->addPiece(bestPieceID, bestX, bestY, bestRotation);

    }
}