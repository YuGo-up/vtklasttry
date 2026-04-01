#pragma once

#include <atomic>

#include <QString>
#include <QStringList>
#include <QHash>

#include <vtkSmartPointer.h>

class vtkDataArray;
class vtkDataSet;

namespace vtklasttry
{
struct ComputeResult
{
    bool ok = false;
    bool canceled = false;
    vtkSmartPointer<vtkDataSet> meshData;
    QHash<QString, vtkSmartPointer<vtkDataArray>> qualityCache; // metric_key -> array
};

// Worker-thread safe: does not access Qt widgets; does not mutate caller's dataset.
ComputeResult computeQualityOnCopy(vtkDataSet* source, const QStringList& metricKeys, const std::atomic<bool>* cancelFlag);

int countSupportedCells(vtkDataSet* ds, const QString& metricKey);
// isFinite()/isMetricSupportedCell() moved to QualitySupport.*

// Pure compute: build "non_ortho" quality array (degrees) for current mesh.
// Name is "Quality" (caller may rename).
vtkSmartPointer<vtkDataArray> computeNonOrthoQualityArray(vtkDataSet* ds, const std::atomic<bool>* cancelFlag);

// Pure compute: build "min_edge_length" quality array for current mesh.
// Name is "Quality" (caller may rename).
vtkSmartPointer<vtkDataArray> computeMinEdgeLengthQualityArray(vtkDataSet* ds, const std::atomic<bool>* cancelFlag);
} // namespace vtklasttry

