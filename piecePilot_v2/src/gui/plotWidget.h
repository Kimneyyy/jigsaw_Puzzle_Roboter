#pragma once
#include <QWidget>
#include <QColor>
#include <vector>

class EdgePoint;

class PlotWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWidget(QWidget* parent = nullptr);

    void setSeries1(std::vector<EdgePoint*> edgePoints);
    void setSeries2(std::vector<EdgePoint*> edgePoints);

    void setInvertSeries2(bool on);
    void setAutoScale(bool on);

    void clear();

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    std::vector<float> m_x1, m_y1;
    std::vector<float> m_x2, m_y2;

    bool m_invert2 = false;
    bool m_invertX = true;
    bool m_autoScale = true;

    // Manuelle Skala (wenn autoscale aus)
    float m_xMin = 0.0, m_xMax = 1.0;
    float m_yMin = -1.0, m_yMax = 1.0;

    QColor m_bg   = QColor(15, 15, 18);
    QColor m_grid = QColor(60, 60, 70);
    QColor m_axis = QColor(160, 160, 170);
    QColor m_text = QColor(210, 210, 220);
    QColor m_s1   = QColor(80, 180, 255);
    QColor m_s2   = QColor(255, 140, 80);

    void computeAutoBounds(float& xMin, float& xMax, float& yMin, float& yMax) const;
};
