#pragma once
#include <QMainWindow>

class QStackedWidget;
class AppController;
class EdgeMatchPage;
class SortGridPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    AppController*  m_ctrl  = nullptr;
    QStackedWidget* m_stack = nullptr;
    EdgeMatchPage*  m_edge  = nullptr;
    SortGridPage*   m_grid  = nullptr;

    void setupMenu();
};
