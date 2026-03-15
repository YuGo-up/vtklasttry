#pragma once

#include <cstdint>
#include <memory>

#include <QtWidgets/QMainWindow>

#include "ui_mainwindow.h"

#include "MeshQualityCalculator.h"

class MeshModel;
class VTKPipeline;

class mainwindow : public QMainWindow
{
    Q_OBJECT

public:
    mainwindow(QWidget* parent = nullptr);
    ~mainwindow();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void on_pushButton_2_clicked();
    void on_pushButton_clicked();
    void on_actionOpenMesh_triggered();
    void on_actionExit_triggered();
    void on_actionResetCamera_triggered();
    void on_actionSetThreshold_triggered();
    void on_actionRecalcQuality_triggered();

private:
    void openMeshAndCompute();
    void recalcQualityAndUpdate();
    void updateCellInfoPanel(int64_t cellId);
    void updateBadCellsLabel();

    Ui::mainwindowClass ui;

    std::unique_ptr<MeshModel>            m_meshModel;
    std::unique_ptr<MeshQualityCalculator> m_qualityCalc;
    std::unique_ptr<VTKPipeline>          m_vtkPipeline;

    QualityData m_qualityData;
    double      m_aspectThreshold = 5.0;
    double      m_skewThreshold   = 0.8;
};
