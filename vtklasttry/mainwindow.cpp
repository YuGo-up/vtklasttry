#include "mainwindow.h"

#include <array>
#include <memory>

#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>

// VTK initialization
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);

#include "MeshModel.h"
#include "MeshQualityCalculator.h"
#include "ThresholdDialog.h"
#include "VTKPipeline.h"

mainwindow::mainwindow(QWidget* parent)
    : QMainWindow(parent)
    , m_meshModel(std::make_unique<MeshModel>())
    , m_qualityCalc(std::make_unique<MeshQualityCalculator>())
    , m_vtkPipeline(std::make_unique<VTKPipeline>())
{
    ui.setupUi(this);

    ui.openGLWidget->SetRenderWindow(m_vtkPipeline->renderWindow());
    ui.openGLWidget->installEventFilter(this);

    ui.labelFileName->setText("-");
    ui.labelBadCells->setText(tr("低质量单元: -"));
    ui.comboBoxMetric->setCurrentIndex(0);
    ui.textEditCellInfo->setPlaceholderText(tr("点击网格选择单元查看指标。"));

    connect(ui.actionOpenMesh, &QAction::triggered, this, &mainwindow::on_actionOpenMesh_triggered);
    connect(ui.actionExit, &QAction::triggered, this, &mainwindow::on_actionExit_triggered);
    connect(ui.actionResetCamera, &QAction::triggered, this, &mainwindow::on_actionResetCamera_triggered);
    connect(ui.actionSetThreshold, &QAction::triggered, this, &mainwindow::on_actionSetThreshold_triggered);
    connect(ui.actionRecalcQuality, &QAction::triggered, this, &mainwindow::on_actionRecalcQuality_triggered);
}

mainwindow::~mainwindow()
{
}

bool mainwindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui.openGLWidget && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton && m_meshModel && m_meshModel->hasMesh())
        {
            int64_t cellId = m_vtkPipeline->pickCell(me->pos().x(), me->pos().y());
            updateCellInfoPanel(cellId);
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void mainwindow::on_pushButton_2_clicked()
{
    openMeshAndCompute();
}

void mainwindow::on_pushButton_clicked()
{
    if (!m_meshModel || !m_meshModel->hasMesh())
    {
        QMessageBox::information(this, tr("提示"), tr("请先打开 STL 网格文件。"));
        return;
    }
    const int metricIndex = ui.comboBoxMetric->currentIndex();
    if (metricIndex == 0)
    {
        if (m_qualityData.aspectMax > m_qualityData.aspectMin)
            m_vtkPipeline->applyQualityScalar("AspectRatio", m_qualityData.aspectMin, m_qualityData.aspectMax);
    }
    else
    {
        if (m_qualityData.skewMax > m_qualityData.skewMin)
            m_vtkPipeline->applyQualityScalar("Skewness", m_qualityData.skewMin, m_qualityData.skewMax);
    }
    updateBadCellsLabel();
}

void mainwindow::on_actionOpenMesh_triggered()
{
    openMeshAndCompute();
}

void mainwindow::on_actionExit_triggered()
{
    close();
}

void mainwindow::on_actionResetCamera_triggered()
{
    m_vtkPipeline->resetCamera();
}

void mainwindow::on_actionSetThreshold_triggered()
{
    ThresholdDialog dlg(m_aspectThreshold, m_skewThreshold, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    m_aspectThreshold = dlg.aspectThreshold();
    m_skewThreshold = dlg.skewThreshold();
    if (m_meshModel && m_meshModel->hasMesh())
        recalcQualityAndUpdate();
}

void mainwindow::on_actionRecalcQuality_triggered()
{
    if (!m_meshModel || !m_meshModel->hasMesh())
    {
        QMessageBox::information(this, tr("提示"), tr("请先打开 STL 网格文件。"));
        return;
    }
    recalcQualityAndUpdate();
}

void mainwindow::openMeshAndCompute()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("选择 STL 网格文件"),
        QString(),
        tr("STL 文件 (*.stl);;所有文件 (*.*)"));

    if (filePath.isEmpty())
        return;

    QString errorMessage;
    if (!m_meshModel->loadFromStl(filePath, errorMessage))
    {
        QMessageBox::warning(this, tr("打开失败"), errorMessage);
        return;
    }

    ui.labelFileName->setText(filePath);
    recalcQualityAndUpdate();
}

void mainwindow::recalcQualityAndUpdate()
{
    if (!m_meshModel || !m_meshModel->hasMesh())
        return;

    m_qualityData = QualityData{};
    const bool ok = m_qualityCalc->computeQuality(
        m_meshModel->polyData(),
        m_qualityData,
        m_aspectThreshold,
        m_skewThreshold);

    if (!ok)
    {
        QMessageBox::warning(this, tr("计算失败"), tr("无法计算网格质量。"));
        return;
    }

    m_vtkPipeline->setMesh(m_meshModel->polyData());

    if (m_qualityData.aspectMax > m_qualityData.aspectMin)
        m_vtkPipeline->applyQualityScalar("AspectRatio", m_qualityData.aspectMin, m_qualityData.aspectMax);

    m_vtkPipeline->updateBadCellsHighlight();
    updateBadCellsLabel();
}

void mainwindow::updateCellInfoPanel(int64_t cellId)
{
    if (cellId < 0)
    {
        ui.textEditCellInfo->setPlainText(tr("点击网格选择单元查看指标。"));
        return;
    }

    const std::size_t idx = static_cast<std::size_t>(cellId);
    if (idx >= m_qualityData.aspectRatio.size() || idx >= m_qualityData.skewness.size())
    {
        ui.textEditCellInfo->setPlainText(tr("单元 %1（无数据）").arg(cellId));
        return;
    }

    QString text;
    text += tr("单元 ID: %1\n").arg(cellId);
    text += tr("长宽比: %1\n").arg(m_qualityData.aspectRatio[idx], 0, 'g', 4);
    text += tr("扭曲度: %1\n").arg(m_qualityData.skewness[idx], 0, 'g', 4);
    ui.textEditCellInfo->setPlainText(text);
}

void mainwindow::updateBadCellsLabel()
{
    ui.labelBadCells->setText(tr("低质量单元: %1").arg(static_cast<int>(m_qualityData.badCells.size())));
}
