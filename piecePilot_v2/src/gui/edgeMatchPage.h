#pragma once
#include <QWidget>

class AppController;

class QTableWidget;
class QComboBox;
class QPushButton;
class QLabel;

class PlotWidget;

class EdgeMatchPage : public QWidget
{
    Q_OBJECT
public:
    explicit EdgeMatchPage(AppController* ctrl, QWidget *parent = nullptr);

private:
    AppController* m_ctrl = nullptr;

    QTableWidget* m_table = nullptr;

    QComboBox* m_pieceA = nullptr;
    QComboBox* m_edgeA  = nullptr;
    QComboBox* m_pieceB = nullptr;
    QComboBox* m_edgeB  = nullptr;

    QPushButton* m_btnMatch = nullptr;
    QLabel* m_lblScore = nullptr;

    PlotWidget* m_plot = nullptr;

    void buildUi();
    void rebuildPieceSelectors();
    void refreshTable();
    void refreshChart();

private slots:
    void onMatchClicked();
};
