#include "mainwindow.h"
#include <QToolBar>
#include <QLabel>
#include <cmath>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Video Grid Viewer");
    resize(1280, 720);
    setStyleSheet("background: #FCFCFC;");

    auto* toolbar = addToolBar("Controls");
    toolbar->addWidget(new QLabel("  Streams: "));

    m_spinBox = new QSpinBox();
    m_spinBox->setRange(1, 9);
    m_spinBox->setValue(4);
    toolbar->addWidget(m_spinBox);

    auto* applyBtn = new QPushButton("Apply Grid");
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::rebuildGrid);
    toolbar->addWidget(applyBtn);

    m_central = new QWidget();
    m_grid = new QGridLayout(m_central);
    m_grid->setSpacing(4);
    setCentralWidget(m_central);

    rebuildGrid();
}

void MainWindow::rebuildGrid() {
    for (auto* w : m_widgets) {
        m_grid->removeWidget(w);
        delete w;
    }
    m_widgets.clear();

    int count = m_spinBox->value();
    int cols = (int)ceil(sqrt((double)count));

    for (int i = 0; i < count; i++) {
        auto* widget = new VideoWidget();
        m_widgets.append(widget);
        m_grid->addWidget(widget, i / cols, i % cols);
    }
}
