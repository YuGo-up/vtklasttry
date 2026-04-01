#pragma once

#include <QString>

class vtkDataArray;
class vtkDataSet;

namespace vtklasttry
{
// Common helpers shared across UI/compute modules.
bool isFinite(double v);
bool isMetricSupportedCell(const QString& metricKey, int vtkCellType);
// Range over supported cells with finite values only.
bool finiteRange(vtkDataSet* ds, vtkDataArray* arr, const QString& metricKey, double outRange[2]);
} // namespace vtklasttry

