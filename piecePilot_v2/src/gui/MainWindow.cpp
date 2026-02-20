#include "MainWindow.h"

#include <QStackedWidget>
#include <QMenuBar>
#include <QAction>
#include <QStatusBar>

#include "../AppController.h"
#include "edgeMatchPage.h"
#include "sortGridPage.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("PiecePilot");
    resize(1200, 700);

    m_ctrl  = new AppController();
    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    m_edge = new EdgeMatchPage(m_ctrl, this);
    m_grid = new SortGridPage(m_ctrl, this);

    m_stack->addWidget(m_edge);
    m_stack->addWidget(m_grid);
    m_stack->setCurrentWidget(m_edge);

    setupMenu();
    statusBar()->showMessage("Bereit");
}

void MainWindow::setupMenu()
{
    auto *viewMenu = menuBar()->addMenu("Ansicht");

    auto *actEdge = new QAction("Kanten-Abgleich", this);
    auto *actGrid = new QAction("Sortier-Grid", this);

    viewMenu->addAction(actEdge);
    viewMenu->addAction(actGrid);

    connect(actEdge, &QAction::triggered, this, [this](){
        m_stack->setCurrentWidget(m_edge);
        statusBar()->showMessage("Kanten-Abgleich");
    });

    connect(actGrid, &QAction::triggered, this, [this](){
        m_stack->setCurrentWidget(m_grid);
        statusBar()->showMessage("Sortier-Grid");
    });

    auto *toolsMenu = menuBar()->addMenu("Tools");
    auto *actRefresh = new QAction("Steuerung aktualisieren", this);
    toolsMenu->addAction(actRefresh);

}
