#pragma once

#include <QHash>
#include <QString>

#include <vtkSmartPointer.h>

class vtkDataArray;
class vtkDataSet;
class vtkScalarBarActor;

namespace vtklasttry
{
struct QualityState
{
    vtkSmartPointer<vtkScalarBarActor> scalarBar;
    vtkSmartPointer<vtkDataSet>* meshData = nullptr;
    vtkSmartPointer<vtkDataArray>* qualityArray = nullptr;
    QHash<QString, vtkSmartPointer<vtkDataArray>>* qualityCache = nullptr;
    QString* lastQualityMetricKey = nullptr;
    QString* lastMetricSupportSummary = nullptr;
};
} // namespace vtklasttry

