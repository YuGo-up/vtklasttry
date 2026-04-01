#include "MeshWorkflow.h"

#include <QFileDialog>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QStatusBar>
#include <QTextEdit>

#include <vtkActor.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDataSetMapper.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkLegendBoxActor.h>
#include <vtkNew.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkTextActor.h>

#include "MeshIO.h"
#include "MeshAnalysis.h"
#include "UiLog.h"

namespace vtklasttry
{
void MeshWorkflow::onSelectFile(QMainWindow* parent,
                                QTextEdit* logBox,
                                QStatusBar* statusBar,
                                const vtklasttry::MeshSelectRequest& req,
                                vtklasttry::MeshState& state)
{
    if (!parent)
        return;

    vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u6253\u5F00\u6587\u4EF6\u9009\u62E9\u6846\u2026"));
    const QString path = QFileDialog::getOpenFileName(
        parent,
        req.openDialogTitle.isEmpty() ? QString::fromWCharArray(L"\u9009\u62E9\u7F51\u683C\u6587\u4EF6") : req.openDialogTitle,
        QString(),
        "All Supported (*.vtk *.vtu *.vtp *.stl *.STL *.inp *.INP);;VTK Legacy (*.vtk);;VTK XML UnstructuredGrid (*.vtu);;VTK XML PolyData (*.vtp);;STL (*.stl *.STL);;Abaqus INP (*.inp *.INP)");
    if (path.isEmpty())
    {
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u53D6\u6D88\u9009\u62E9\u6587\u4EF6\u3002"));
        return;
    }

    vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u9009\u4E2D\u6587\u4EF6\uFF1A%1").arg(path));

    QString pipeline;
    vtkSmartPointer<vtkDataSet> ds;
    QString meshClassText;
    QString cellTypeStatsMd;
    if (!vtklasttry::MeshWorkflow::loadMesh(parent,
                                            logBox,
                                            statusBar,
                                            state.renderer,
                                            state.renderWindow,
                                            path,
                                            &pipeline,
                                            &ds,
                                            &meshClassText,
                                            &cellTypeStatsMd,
                                            state.meshActor,
                                            state.scalarBar,
                                            state.comboLegend,
                                            state.comboLegendTitle))
    {
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u52A0\u8F7D\u5931\u8D25\u3002"));
        return;
    }

    if (state.currentFilePath)
        *state.currentFilePath = path;
    if (state.lastImportPipelineText)
        *state.lastImportPipelineText = pipeline;
    if (state.meshData)
        *state.meshData = ds;

    if (state.qualityArray)
        *state.qualityArray = nullptr;
    if (state.lastQualityMetricKey)
        state.lastQualityMetricKey->clear();
    if (state.qualityCache)
        state.qualityCache->clear();
    if (state.selectedCellId)
        *state.selectedCellId = -1;
    if (state.isolatedActor)
        state.isolatedActor->SetVisibility(false);
    if (state.lastBadCellIds)
        state.lastBadCellIds->clear();
    if (state.lastTotalCellsForStats)
        *state.lastTotalCellsForStats = ds ? static_cast<int>(ds->GetNumberOfCells()) : 0;
    if (state.badStatsLabel)
        state.badStatsLabel->setText(QString::fromWCharArray(L"\u574F\u5355\u5143\u7EDF\u8BA1\uFF1A-"));

    if (state.lastMeshClassText)
        *state.lastMeshClassText = meshClassText;
    if (state.lastCellTypeStatsTableMd)
        *state.lastCellTypeStatsTableMd = cellTypeStatsMd;

    if (statusBar)
        statusBar->showMessage(QString::fromWCharArray(L"\u5DF2\u52A0\u8F7D\uFF1A%1").arg(path));
    if (state.renderer)
        state.renderer->ResetCamera();
    if (state.renderWindow)
        state.renderWindow->Render();
}

bool MeshWorkflow::loadMesh(QMainWindow* parent,
                            QTextEdit* logBox,
                            QStatusBar* statusBar,
                            vtkSmartPointer<vtkRenderer> renderer,
                            vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow,
                            const QString& path,
                            QString* outPipelineText,
                            vtkSmartPointer<vtkDataSet>* outMeshData,
                            QString* outMeshClassText,
                            QString* outCellTypeStatsTableMd,
                            vtkSmartPointer<vtkActor>* inOutMeshActor,
                            vtkSmartPointer<vtkScalarBarActor> scalarBar,
                            vtkSmartPointer<vtkLegendBoxActor> comboLegend,
                            vtkSmartPointer<vtkTextActor> comboLegendTitle)
{
    if (!parent)
        return false;

    if (outPipelineText)
        outPipelineText->clear();
    const auto res = vtklasttry::loadMesh(path, [&](const QString& s) { vtklasttry::appendUiLog(logBox, s); });
    if (outPipelineText)
        *outPipelineText = res.pipeline;
    vtkSmartPointer<vtkDataSet> ds = res.data;

    if (!res.ok || !ds || ds->GetNumberOfCells() == 0)
    {
        QMessageBox::warning(
            parent,
            QString::fromWCharArray(L"\u52A0\u8F7D\u5931\u8D25"),
            res.message.isEmpty() ? QString::fromWCharArray(L"\u8BFB\u53D6\u5230\u7684\u7F51\u683C\u4E3A\u7A7A\uFF0C\u6216\u6587\u4EF6\u683C\u5F0F\u4E0D\u6B63\u786E\u3002") : res.message);
        return false;
    }

    if (outMeshData)
        *outMeshData = ds;

    // Mesh classification (based on vtkCellType statistics)
    {
        const vtklasttry::MeshAnalysisResult ar = vtklasttry::analyzeMesh(ds);
        if (outMeshClassText)
            *outMeshClassText = ar.meshClassText;
        if (outCellTypeStatsTableMd)
            *outCellTypeStatsTableMd = ar.cellTypeStatsTableMd;
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u7F51\u683C\u5206\u7C7B\uFF1A%1").arg(ar.meshClassText));
    }
    if (statusBar)
    {
        statusBar->showMessage(QString::fromWCharArray(L"\u7F51\u683C\u5DF2\u52A0\u8F7D\uFF1Apoints=%1, cells=%2")
                                   .arg(ds->GetNumberOfPoints())
                                   .arg(ds->GetNumberOfCells()));
    }

    // surface actor
    vtkNew<vtkDataSetSurfaceFilter> surface;
    surface->SetInputData(ds);
    surface->Update();

    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputConnection(surface->GetOutputPort());
    mapper->ScalarVisibilityOff();

    if (!inOutMeshActor)
        return false;
    if (!(*inOutMeshActor))
        *inOutMeshActor = vtkSmartPointer<vtkActor>::New();
    (*inOutMeshActor)->SetMapper(mapper);
    (*inOutMeshActor)->GetProperty()->SetColor(0.85, 0.85, 0.85);
    (*inOutMeshActor)->GetProperty()->SetOpacity(1.0);
    (*inOutMeshActor)->GetProperty()->SetRepresentationToSurface();
    (*inOutMeshActor)->GetProperty()->EdgeVisibilityOn();
    (*inOutMeshActor)->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
    (*inOutMeshActor)->GetProperty()->SetLineWidth(1.0);

    if (renderer)
    {
        renderer->RemoveAllViewProps();
        renderer->AddActor(*inOutMeshActor);
        if (scalarBar)
            renderer->AddActor2D(scalarBar);
        // RemoveAllViewProps() also removes our custom 2D legend actors; re-add them here.
        if (comboLegend)
            renderer->AddActor2D(comboLegend);
        if (comboLegendTitle)
            renderer->AddActor2D(comboLegendTitle);
        renderer->ResetCamera();
    }
    if (renderWindow)
        renderWindow->Render();
    return true;
}
} // namespace vtklasttry

