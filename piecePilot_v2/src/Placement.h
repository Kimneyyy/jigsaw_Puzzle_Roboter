#ifndef PLACEMENT_H
#define PLACEMENT_H

class Placement
{
public:
    Placement() {};
    ~Placement() {};

    int x = 0;
    int y = 0;
    int rotation = 0; // 0, 90, 180, 270  -. rotiert immer nach rechts

    int pieceID = -1; // welches Teil liegt hier?


};

#endif // PLACEMENT_H