#ifndef BOARD_H
#define BOARD_H
#include <vector>

class Placement;
class PieceInfo;
class AppController;

struct bestNextPlace{
    int pieceID;
    int edgeID;
    double matchScore;
    int x;
    int y;
    int positionRelativeToNeighbor; // 0=oben, 1=rechts, 2=unten, 3=links
};

class Board
{
public:
    Board(AppController* appController) {this->appController = appController; };
    ~Board();

    AppController* appController = nullptr;
    void addPiece(int pieceID, int x, int y, int rotation);
    Placement* getPlacement(int x, int y);
    std::vector<Placement*> getPlacements() { return placements; }
    Placement* getPieceByID(int pieceID);

    bestNextPlace* getBestNextPlace(int pieceID, int edgeID, std::vector<PieceInfo*> pieces);

    void normalizeBoardToZero();

private:

    std::vector<Placement*> placements;
};

#endif // BOARD_H