#include "sortGridPage.h"
#include "../AppController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QSpinBox>
#include <QLabel>
#include "../pieceInfo.h"
#include "../Board.h"
#include "../Placement.h"

SortGridPage::SortGridPage(AppController* ctrl, QWidget *parent)
    : QWidget(parent), m_ctrl(ctrl)
{
    buildUi();
    rebuildGrid();
}

void SortGridPage::buildUi()
{
    auto *root = new QVBoxLayout(this);

    auto *gbTop = new QGroupBox("Sortier-Grid", this);
    auto *topLay = new QHBoxLayout(gbTop);

    m_cols = new QSpinBox(this);
    m_rows = new QSpinBox(this);
    m_cols->setRange(1, 50);
    m_rows->setRange(1, 50);
    m_cols->setValue(3);
    m_rows->setValue(2);

    topLay->addWidget(new QLabel("Spalten:", this));
    topLay->addWidget(m_cols);
    topLay->addSpacing(12);
    topLay->addWidget(new QLabel("Zeilen:", this));
    topLay->addWidget(m_rows);
    topLay->addStretch(1);

    root->addWidget(gbTop, 0);

    m_scene = new QGraphicsScene(this);
    m_view  = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);

    root->addWidget(m_view, 1);

    connect(m_cols, qOverload<int>(&QSpinBox::valueChanged), this, &SortGridPage::onGridChanged);
    connect(m_rows, qOverload<int>(&QSpinBox::valueChanged), this, &SortGridPage::onGridChanged);
}

void SortGridPage::onGridChanged()
{
    rebuildGrid();
}

void SortGridPage::rebuildGrid()
{
    m_scene->clear();

    const int cols = m_cols->value();
    const int rows = m_rows->value();

    const int cell = 70;        // px
    const int margin = 20;

    const int w = cols * cell;
    const int h = rows * cell;

    // Hintergrund
    m_scene->setSceneRect(0, 0, w + 2 * margin, h + 2 * margin);

    // Grid zeichnen
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            QRectF r(margin + x * cell, margin + y * cell, cell, cell);
            auto *rect = m_scene->addRect(r);
            rect->setOpacity(0.25);
        }
    }




    Board* board = m_ctrl->getBoard();
    board->normalizeBoardToZero(); // damit die Teile nicht irgendwo im negativen Bereich liegen, sondern um (0,0) zentriert sind
    std::vector<Placement*> placements = board->getPlacements();
    for(int i = 0; i < placements.size(); i++){
        Placement* placement = placements[i];
        int gx = placement->x;
        int gy = placement->y;
        PieceInfo* piece = m_ctrl->getPieces()[placement->pieceID];

        QRectF pr(margin + gx * cell + 8, margin + gy * cell + 8, cell - 16, cell - 16);
        auto *p = m_scene->addRect(pr);
        p->setOpacity(0.8);
        p->setToolTip(piece->name + QString(" (ID: %1)").arg(piece->id) + QString(", x: %1, y: %2, rotation: %3°, array_position: %4, matchScore: %5").arg(placement->x).arg(placement->y).arg(placement->rotation).arg(i).arg(placement->matchScore));
        p->setFlag(QGraphicsItem::ItemIsMovable, true);
        p->setFlag(QGraphicsItem::ItemIsSelectable, true);
        // Bild von piece->imagePath als Hintergrund des Rechteckes einfügen
        // Bild auf die Größe des Rechtecks skalieren (mit Aspect Ratio beibehalten)
        if (!piece->imagePath.isEmpty()) {
            QPixmap pix(piece->imagePath);
            if (!pix.isNull()) {
                pix = pix.scaled(pr.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                auto *pixItem = m_scene->addPixmap(pix);
                pixItem->setPos(pr.topLeft());
                pixItem->setOpacity(0.8);
                pixItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                pixItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                // Drehung um die Mitte
                pixItem->setTransformOriginPoint(pixItem->boundingRect().center());
                pixItem->setRotation(-placement->rotation); // Grad: 90, 180, 270, ...
            }
        }
    }


    /* std::vector<PieceInfo*> pieces = m_ctrl->getPieces();
    for(int i = 0; i < pieces.size(); i++)
    {
        int gx = i % cols;
        int gy = (i / cols) % rows;

        QRectF pr(margin + gx * cell + 8, margin + gy * cell + 8, cell - 16, cell - 16);
        auto *p = m_scene->addRect(pr);
        p->setOpacity(0.8);
        p->setToolTip(pieces[i]->name);
        p->setFlag(QGraphicsItem::ItemIsMovable, true);
        p->setFlag(QGraphicsItem::ItemIsSelectable, true);
        // Bild von piece->imagePath als Hintergrund des Rechteckes einfügen
        // Bild auf die Größe des Rechtecks skalieren (mit Aspect Ratio beibehalten)
        if (!pieces[i]->imagePath.isEmpty()) {
            QPixmap pix(pieces[i]->imagePath);
            if (!pix.isNull()) {
                pix = pix.scaled(pr.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                auto *pixItem = m_scene->addPixmap(pix);
                pixItem->setPos(pr.topLeft());
                pixItem->setOpacity(0.8);
                pixItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                pixItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
            }
        }
    } */

    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}
