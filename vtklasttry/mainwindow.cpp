#include "mainwindow.h"

#include <array>
#include <memory>

#include <QFileDialog>
#include <QMessageBox>

// VTK initialization
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);

#include "MeshModel.h"
#include "MeshQualityCalculator.h"
#include "VTKPipeline.h"

namespace
{
    // Default quality thresholds
    constexpr double DEFAULT_ASPECT_THRESHOLD = 5.0;
    constexpr double DEFAULT_SKEW_THRESHOLD   = 0.8;
}

mainwindow::mainwindow(QWidget* parent)
    : QMainWindow(parent)
    , m_meshModel(std::make_unique<MeshModel>())
    , m_qualityCalc(std::make_unique<MeshQualityCalculator>())
    , m_vtkPipeline(std::make_unique<VTKPipeline>())
{
    ui.setupUi(this);

    // Bind VTK render window to Qt widget
    ui.openGLWidget->SetRenderWindow(m_vtkPipeline->renderWindow());

    ui.labelFileName->setText("-");
    ui.labelBadCells->setText(tr("Low-quality cells: -"));
    ui.comboBoxMetric->setCurrentIndex(0);
}

mainwindow::~mainwindow()
{
}

void mainwindow::on_pushButton_2_clicked()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Open STL mesh file"),
        QString(),
        tr("STL files (*.stl);;All files (*.*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    QString errorMessage;
    if (!m_meshModel->loadFromStl(filePath, errorMessage))
    {
        QMessageBox::warning(this, tr("Open failed"), errorMessage);
        return;
    }

    ui.labelFileName->setText(filePath);

    // Compute mesh quality
    m_qualityData = QualityData{};
    const bool ok = m_qualityCalc->computeQuality(
        m_meshModel->polyData(),
        m_qualityData,
        DEFAULT_ASPECT_THRESHOLD,
        DEFAULT_SKEW_THRESHOLD);

    if (!ok)
    {
        QMessageBox::warning(this, tr("Compute failed"), tr("Failed to compute mesh quality."));
        return;
    }

    // Update VTK mesh
    m_vtkPipeline->setMesh(m_meshModel->polyData());

    // Default: color by aspect ratio
    if (m_qualityData.aspectMax > m_qualityData.aspectMin)
    {
        m_vtkPipeline->applyQualityScalar(
            "AspectRatio",
            m_qualityData.aspectMin,
            m_qualityData.aspectMax);
    }

    ui.labelBadCells->setText(
        tr("Low-quality cells: %1").arg(static_cast<int>(m_qualityData.badCells.size())));
}

void mainwindow::on_pushButton_clicked()
{
    if (!m_meshModel || !m_meshModel->hasMesh())
    {
        QMessageBox::information(this, tr("Info"), tr("Please open an STL mesh first."));
        return;
    }

    const int metricIndex = ui.comboBoxMetric->currentIndex();
    if (metricIndex == 0)
    {
        // Aspect ratio
        if (m_qualityData.aspectMax > m_qualityData.aspectMin)
        {
            m_vtkPipeline->applyQualityScalar(
                "AspectRatio",
                m_qualityData.aspectMin,
                m_qualityData.aspectMax);
        }
    }
    else
    {
        // Skewness
        if (m_qualityData.skewMax > m_qualityData.skewMin)
        {
            m_vtkPipeline->applyQualityScalar(
                "Skewness",
                m_qualityData.skewMin,
                m_qualityData.skewMax);
        }
    }

    ui.labelBadCells->setText(
        tr("Low-quality cells: %1").arg(static_cast<int>(m_qualityData.badCells.size())));
}

