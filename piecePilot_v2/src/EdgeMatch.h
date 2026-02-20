#ifndef EDGEMATCH_H
#define EDGEMATCH_H


class EdgeMatch
{
public:
    EdgeMatch() {};
    ~EdgeMatch();


public:
    float score; // je kleiner, desto besser
    int pieceID = -1;
    int edgeID = -1;
};

#endif // EDGEMATCH_H
