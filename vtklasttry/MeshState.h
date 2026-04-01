#pragma once

#include <QHash>
#include <QVector>
#include <QString>

#include <vtkSmartPointer.h>

class QLabel;
class vtkActor;
class vtkDataArray;
class vtkDataSet;
class vtkGenericOpenGLRenderWindow;
class vtkLegendBoxActor;
class vtkRenderer;
class vtkScalarBarActor;
class vtkTextActor;
using vtkIdType = long long;

namespace vtklasttry
{
struct MeshState
{
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;

    QString* currentFilePath = nullptr;
    QString* lastImportPipelineText = nullptr;
    vtkSmartPointer<vtkDataSet>* meshData = nullptr;
    vtkSmartPointer<vtkDataArray>* qualityArray = nullptr;
    QString* lastQualityMetricKey = nullptr;
    QHash<QString, vtkSmartPointer<vtkDataArray>>* qualityCache = nullptr;
    vtkIdType* selectedCellId = nullptr;
    vtkSmartPointer<vtkActor> isolatedActor;
    QVector<vtkIdType>* lastBadCellIds = nullptr;
    int* lastTotalCellsForStats = nullptr;
    QLabel* badStatsLabel = nullptr;
    QString* lastMeshClassText = nullptr;
    QString* lastCellTypeStatsTableMd = nullptr;
    vtkSmartPointer<vtkActor>* meshActor = nullptr;
    vtkSmartPointer<vtkScalarBarActor> scalarBar;
    vtkSmartPointer<vtkLegendBoxActor> comboLegend;
    vtkSmartPointer<vtkTextActor> comboLegendTitle;
};
} // namespace vtklasttry

