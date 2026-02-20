#pragma once
#include <QObject>
#include <QString>
#include <vector>

class PieceScan;
class PieceInfo;
class PieceEdgeProfile;
class Board;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController();

    std::vector<PieceInfo*> pieces() const { return pieceInfo; }

    void scanPiece(const std::string& imagePath);

    PieceEdgeProfile* getEdgeProfile(int pieceID, int edgeID);
    PieceEdgeProfile* getInvertedEdgeProfile(int pieceID, int edgeID);

    double matchEdges(int pieceA_id, int edgeA_id, int pieceB_id, int edgeB_id);
    PieceEdgeProfile* invertEdgeProfile(PieceEdgeProfile* prof);

    void computeAllEdgeMatches();
    void sortPieceToBoard();

    std::vector<PieceInfo*> getPieces();

    PieceInfo* getBestPieceToStartBoard();

    Board* getBoard() { return board; }

private:
    std::vector<PieceInfo*> pieceInfo = std::vector<PieceInfo*>();
    PieceScan* pieceScanner = nullptr;

    Board* board = nullptr;
    //void generateDummyPieces();
    QString basePath;
};
