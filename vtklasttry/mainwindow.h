#pragma once

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
    mainwindow(QWidget *parent = nullptr);
    ~mainwindow();

private slots:
    void on_pushButton_2_clicked();   // 选择网格文件
    void on_pushButton_clicked();     // 显示/更新质量可视化

private:
    Ui::mainwindowClass ui;

    std::unique_ptr<MeshModel>          m_meshModel;
    std::unique_ptr<MeshQualityCalculator> m_qualityCalc;
    std::unique_ptr<VTKPipeline>        m_vtkPipeline;

    QualityData                         m_qualityData;
};
