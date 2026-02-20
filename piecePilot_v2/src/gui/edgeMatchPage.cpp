#include "edgeMatchPage.h"
#include "../AppController.h"
#include "plotWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

#include <cmath>
#include "../pieceInfo.h"

static QString edgeName(int idx)
{
    switch (idx) {
        case 0: return "Oben";
        case 1: return "Rechts";
        case 2: return "Unten";
        case 3: return "Links";
        default: return "Kante";
    }
}

EdgeMatchPage::EdgeMatchPage(AppController* ctrl, QWidget *parent)
    : QWidget(parent), m_ctrl(ctrl)
{
    buildUi();

    // Wenn du im Controller Signals hast:
    /* connect(m_ctrl, &AppController::pieceDataUpdated, this, [this](){
        rebuildPieceSelectors();
        refreshTable();
        refreshChart();
    }); */

    rebuildPieceSelectors();
    refreshTable();
    refreshChart();
}

void EdgeMatchPage::buildUi()
{
    auto *root = new QVBoxLayout(this);

    // ========== OBEN: Teile-Liste ==========
    auto *gbList = new QGroupBox("Teile-Liste", this);
    auto *listLay = new QVBoxLayout(gbList);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Teil", "Oben", "Rechts", "Unten", "Links"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    listLay->addWidget(m_table);
    root->addWidget(gbList, 2);

    // ========== MITTE: Auswahl + Match ==========
    auto *gbSel = new QGroupBox("Kanten-Abgleich", this);
    auto *selLay = new QHBoxLayout(gbSel);

    m_pieceA = new QComboBox(this);
    m_edgeA  = new QComboBox(this);
    m_pieceB = new QComboBox(this);
    m_edgeB  = new QComboBox(this);

    for (int i = 0; i < 4; ++i) {
        m_edgeA->addItem(edgeName(i), i);
        m_edgeB->addItem(edgeName(i), i);
    }

    m_btnMatch = new QPushButton("Match", this);
    m_lblScore = new QLabel("Übereinstimmung: -", this);

    selLay->addWidget(new QLabel("Kante 1:", this));
    selLay->addWidget(m_pieceA);
    selLay->addWidget(m_edgeA);

    selLay->addSpacing(16);

    selLay->addWidget(new QLabel("Kante 2:", this));
    selLay->addWidget(m_pieceB);
    selLay->addWidget(m_edgeB);

    selLay->addStretch(1);
    selLay->addWidget(m_lblScore);
    selLay->addWidget(m_btnMatch);

    root->addWidget(gbSel, 0);

    connect(m_btnMatch, &QPushButton::clicked, this, &EdgeMatchPage::onMatchClicked);

    auto refresh = [this](){ refreshChart(); };
    connect(m_pieceA, &QComboBox::currentIndexChanged, this, refresh);
    connect(m_edgeA,  &QComboBox::currentIndexChanged, this, refresh);
    connect(m_pieceB, &QComboBox::currentIndexChanged, this, refresh);
    connect(m_edgeB,  &QComboBox::currentIndexChanged, this, refresh);

    // Komplementär oder gleich vergleichen?

    // ========== UNTEN: Plot (2 Kurven) ==========
    auto *gbPlot = new QGroupBox("Profil-Vergleich (2 Kurven)", this);
    auto *plotLay = new QVBoxLayout(gbPlot);

    m_plot = new PlotWidget(this);
    m_plot->setAutoScale(true);
    m_plot->setInvertSeries2(true); // für Puzzle-Kanten oft sinnvoll (Komplement)

    plotLay->addWidget(m_plot);
    root->addWidget(gbPlot, 2);
}

void EdgeMatchPage::rebuildPieceSelectors()
{
    m_pieceA->blockSignals(true);
    m_pieceB->blockSignals(true);

    m_pieceA->clear();
    m_pieceB->clear();

    std::vector<PieceInfo*> pcs = m_ctrl->pieces();
    for(PieceInfo* p : pcs) {
        if (!p) continue;
        m_pieceA->addItem(p->name, p->id);
        m_pieceB->addItem(p->name, p->id);
    }

    if (m_pieceA->count() > 0) m_pieceA->setCurrentIndex(0);
    if (m_pieceB->count() > 1) m_pieceB->setCurrentIndex(1);

    m_pieceA->blockSignals(false);
    m_pieceB->blockSignals(false);
}

void EdgeMatchPage::refreshTable()
{
    const auto &pcs = m_ctrl->pieces();
    m_table->setRowCount((int)pcs.size());

    for (int r = 0; r < (int)pcs.size(); ++r) {
        const auto &p = pcs[r];

        auto *itemName = new QTableWidgetItem(p->name);
        m_table->setItem(r, 0, itemName);

        // Platzhalter pro Kante – später kannst du hier Mini-Previews/IDs reinmachen
        for (int e = 0; e < 4; ++e) {
            QString edgeText = QString("K%1").arg(e);
            edgeText += (p->edgeProfile[e]->edgeType == 0) ? " (gerade)" :
                        (p->edgeProfile[e]->edgeType == 1) ? " (weiblich)" :
                        (p->edgeProfile[e]->edgeType == 2) ? " (männlich)" : " (?)";
            auto *it = new QTableWidgetItem(edgeText);
            it->setTextAlignment(Qt::AlignCenter);
            m_table->setItem(r, e + 1, it);
        }
    }

    m_table->resizeColumnsToContents();
}

void EdgeMatchPage::refreshChart()
{
    if (!m_plot) return;
    if (m_pieceA->count() == 0 || m_pieceB->count() == 0) return;

    const int pieceA = m_pieceA->currentData().toInt();
    const int edgeA  = m_edgeA->currentData().toInt();
    const int pieceB = m_pieceB->currentData().toInt();
    const int edgeB  = m_edgeB->currentData().toInt();

    PieceEdgeProfile* aProf = m_ctrl->getEdgeProfile(pieceA, edgeA);
    PieceEdgeProfile* bProf = m_ctrl->getInvertedEdgeProfile(pieceB, edgeB);

    m_plot->setSeries1(aProf->edgePoints);
    m_plot->setSeries2(bProf->edgePoints);

    // Wenn du mal “direkt” statt komplementär vergleichen willst:
    m_plot->setInvertSeries2(false);
}


void EdgeMatchPage::onMatchClicked()
{
    const int pieceA = m_pieceA->currentData().toInt();
    const int edgeA  = m_edgeA->currentData().toInt();
    const int pieceB = m_pieceB->currentData().toInt();
    const int edgeB  = m_edgeB->currentData().toInt();

    const double score = m_ctrl->matchEdges(pieceA, edgeA, pieceB, edgeB);
    m_lblScore->setText(QString("Übereinstimmung: %1%").arg((int)std::round(score * 100.0)));

    refreshChart();
}
