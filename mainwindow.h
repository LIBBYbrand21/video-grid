#pragma once
#include <QMainWindow>
#include <QGridLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QVector>
#include "videowidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void rebuildGrid();

private:
    QWidget* m_central;
    QGridLayout* m_grid;
    QSpinBox* m_spinBox;
    QVector<VideoWidget*> m_widgets;
};