#pragma once
#include <QWidget>

class AppController;

class QGraphicsView;
class QGraphicsScene;
class QSpinBox;

class SortGridPage : public QWidget
{
    Q_OBJECT
public:
    explicit SortGridPage(AppController* ctrl, QWidget *parent = nullptr);

    void calcCoverTransform(const QSizeF& src, const QSizeF& dst, qreal& scale, QPointF& offset);

private:
    AppController* m_ctrl = nullptr;

    QGraphicsView*  m_view  = nullptr;
    QGraphicsScene* m_scene = nullptr;

    QSpinBox* m_cols = nullptr;
    QSpinBox* m_rows = nullptr;

    void buildUi();
    void rebuildGrid();

private slots:
    void onGridChanged();
};
