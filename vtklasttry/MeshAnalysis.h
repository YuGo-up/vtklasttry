#pragma once

#include <QString>

#include <vtkSmartPointer.h>

class vtkDataSet;

namespace vtklasttry
{
struct MeshAnalysisResult
{
    QString meshClassText;          // e.g. "体网格（3D）"
    QString cellTypeStatsTableMd;   // markdown rows (no header)
};

MeshAnalysisResult analyzeMesh(vtkDataSet* ds);

// For UI/log/report: summarize which cell types in current mesh support the metric.
// Format: "VTK_QUAD:123, VTK_TRIANGLE:45, ..."
QString buildMetricSupportSummary(vtkDataSet* ds, const QString& metricKey);
} // namespace vtklasttry

