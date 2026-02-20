#include "plotWidget.h"
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>
#include <QPainterPath>
#include "../PieceEdgeProfile.h"
#include "../EdgePoint.h"

PlotWidget::PlotWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(450, 300);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, m_bg);
    setPalette(pal);
}

void PlotWidget::setSeries1(std::vector<EdgePoint*> edgePoints)
{
    m_x1.clear();
    m_y1.clear();

    for(EdgePoint* ep : edgePoints) {
        m_x1.push_back(ep->x);
        m_y1.push_back(ep->y);
    }

    update();
}

void PlotWidget::setSeries2(std::vector<EdgePoint*> edgePoints)
{
    m_x2.clear();
    m_y2.clear();

    for(EdgePoint* ep : edgePoints) {
        m_x2.push_back(ep->x);
        m_y2.push_back(ep->y);
    }

    update();
}

void PlotWidget::setInvertSeries2(bool on)
{
    m_invert2 = on;
    update();
}

void PlotWidget::setAutoScale(bool on)
{
    m_autoScale = on;
    update();
}

void PlotWidget::clear()
{
    m_x1.clear(); m_y1.clear();
    m_x2.clear(); m_y2.clear();
    update();
}

void PlotWidget::computeAutoBounds(float& xMin, float& xMax, float& yMin, float& yMax) const
{
    bool have = false;

    auto absorb = [&](const std::vector<float>& x, const std::vector<float>& y, bool invertY)
    {
        if (x.empty() || y.empty()) return;
        size_t n = std::min(x.size(), y.size());
        if (n == 0) return;

        for (size_t i = 0; i < n; ++i) {
            float xx = x[i];
            float yy = invertY ? -y[i] : y[i];
            if (!std::isfinite(xx) || !std::isfinite(yy)) continue;

            if (!have) {
                xMin = xMax = xx;
                yMin = yMax = yy;
                have = true;
            } else {
                xMin = std::min(xMin, xx);
                xMax = std::max(xMax, xx);
                yMin = std::min(yMin, yy);
                yMax = std::max(yMax, yy);
            }
        }
    };

    absorb(m_x1, m_y1, false);
    absorb(m_x2, m_y2, m_invert2);

    if (!have) {
        xMin = 0.0; xMax = 1.0;
        yMin = -1.0; yMax = 1.0;
        return;
    }

    // Padding
    float dx = (xMax - xMin);
    float dy = (yMax - yMin);
    if (dx <= 1e-12) { xMin -= 1.0; xMax += 1.0; }
    if (dy <= 1e-12) { yMin -= 1.0; yMax += 1.0; }

    dx = (xMax - xMin);
    dy = (yMax - yMin);

    xMin -= 0.05 * dx; xMax += 0.05 * dx;
    yMin -= 0.10 * dy; yMax += 0.10 * dy;
}

void PlotWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int W = width();
    const int H = height();

    // Plot area margins
    const int left   = 55;
    const int right  = 20;
    const int top    = 20;
    const int bottom = 35;

    QRect plot(left, top, W - left - right, H - top - bottom);
    if (plot.width() <= 10 || plot.height() <= 10) return;

    // Bounds
    float xMin = m_xMin, xMax = m_xMax, yMin = m_yMin, yMax = m_yMax;
    if (m_autoScale) computeAutoBounds(xMin, xMax, yMin, yMax);

    auto xToPx = [&](float x) {
        float t = (x - xMin) / (xMax - xMin);
        return plot.left() + t * plot.width();
    };
    auto yToPx = [&](float y) {
        float t = (y - yMin) / (yMax - yMin);
        return plot.bottom() - t * plot.height();
    };

    // Grid
    p.setPen(QPen(m_grid, 1));
    const int gridX = 8;
    const int gridY = 6;

    for (int i = 0; i <= gridX; ++i) {
        float t = (float)i / gridX;
        int x = plot.left() + (int)std::round(t * plot.width());
        p.drawLine(x, plot.top(), x, plot.bottom());
    }
    for (int i = 0; i <= gridY; ++i) {
        float t = (float)i / gridY;
        int y = plot.top() + (int)std::round(t * plot.height());
        p.drawLine(plot.left(), y, plot.right(), y);
    }

    // Axes border
    p.setPen(QPen(m_axis, 1.5));
    p.drawRect(plot);

    // Labels
    p.setPen(m_text);
    QFontMetrics fm(p.font());
    p.drawText(10, top + fm.ascent(), "y");
    p.drawText(plot.right() - 5, H - 10, "x");

    // y ticks
    auto drawTickLabelY = [&](float yVal, int py) {
        QString s = QString::number(yVal, 'f', 2);
        int tw = fm.horizontalAdvance(s);
        p.drawText(left - 8 - tw, py + fm.ascent()/2, s);
        p.drawLine(left - 5, py, left, py);
    };
    for (int i = 0; i <= gridY; ++i) {
        float t = (float)i / gridY;
        float yVal = yMax - t * (yMax - yMin);
        int py = plot.top() + (int)std::round(t * plot.height());
        drawTickLabelY(yVal, py);
    }

    // x ticks
    auto drawTickLabelX = [&](float xVal, int px) {
        QString s = QString::number(xVal, 'f', 2);
        int tw = fm.horizontalAdvance(s);
        p.drawText(px - tw/2, plot.bottom() + fm.height() + 2, s);
        p.drawLine(px, plot.bottom(), px, plot.bottom() + 5);
    };
    for (int i = 0; i <= gridX; ++i) {
        float t = (float)i / gridX;
        float xVal = xMin + t * (xMax - xMin);
        int px = plot.left() + (int)std::round(t * plot.width());
        drawTickLabelX(xVal, px);
    }

    auto drawSeries = [&](const std::vector<float>& x,
                      const std::vector<float>& y,
                      const QColor& col,
                      bool invertY,
                      bool mirrorX)   // <-- neu
    {
        size_t n = std::min(x.size(), y.size());
        if (n < 2) return;

        QPainterPath path;
        bool started = false;

        for (size_t i = 0; i < n; ++i) {
            float xx = x[i];
            float yy = invertY ? -y[i] : y[i];
            if (!std::isfinite(xx) || !std::isfinite(yy)) continue;

            float px = xToPx(xx);
            if (mirrorX) {
                px = plot.left() + plot.right() - px; // <-- Spiegelung nur für diese Serie
            }

            float py = yToPx(yy);

            if (!started) {
                path.moveTo(px, py);
                started = true;
            } else {
                path.lineTo(px, py);
            }
        }

        p.setPen(QPen(col, 2.0));
        p.drawPath(path);
    };


    // Series
    drawSeries(m_x1, m_y1, m_s1, false, false);
    //drawSeries(m_x2, m_y2, m_s2, m_invert2, m_invertX);   // <-- Option B: Spiegeln der x-Achse für die 2. Serie
    drawSeries(m_x2, m_y2, m_s2, false, false);

    // Legend (simple)
    p.setPen(m_s1);
    p.drawText(plot.left() + 10, plot.top() + 18, "Kante 1");
    p.setPen(m_s2);
    p.drawText(plot.left() + 90, plot.top() + 18, m_invert2 ? "Kante 2 (invert)" : "Kante 2");
}
