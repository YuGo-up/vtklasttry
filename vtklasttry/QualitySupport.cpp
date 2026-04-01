#include "QualitySupport.h"

#include <cmath>

#include <vtkDataArray.h>
#include <vtkDataSet.h>

namespace vtklasttry
{
bool isFinite(double v)
{
    return std::isfinite(v) != 0;
}

bool isMetricSupportedCell(const QString& metricKey, int ct)
{
    // Keep it explicit and conservative to avoid reporting nonsense for unsupported topologies.
    // Extend here when you add measures for wedge/pyramid/polyhedron etc.
    if (metricKey == "aspect")
        return (ct == 5 || ct == 9 || ct == 10 || ct == 12); // TRI, QUAD, TETRA, HEX
    if (metricKey == "skew")
        return (ct == 5 || ct == 9 || ct == 10 || ct == 12); // TRI uses Shape&Size; TET uses ScaledJacobian
    if (metricKey == "scaled_jacobian")
        return (ct == 5 || ct == 9 || ct == 10 || ct == 12);
    if (metricKey == "non_ortho")
        return (ct == 10 || ct == 12 || ct == 13 || ct == 14); // TETRA/HEX/WEDGE/PYRAMID (3D cells)
    if (metricKey == "jacobian")
        return (ct == 5 || ct == 9 || ct == 10 || ct == 12); // TRI: uses Scaled Jacobian as 2D surrogate
    if (metricKey == "jacobian_ratio")
        return (ct == 5 || ct == 9 || ct == 10 || ct == 12); // Verdict edge ratio (max/min edge), >=1
    if (metricKey == "min_angle")
        return (ct == 5 || ct == 9 || ct == 10); // degrees; VTK has no hex min-angle helper in 8.2
    if (metricKey == "max_angle")
        return (ct == 5 || ct == 9); // degrees (surface); tet max-angle not exposed in vtkMeshQuality 8.2
    if (metricKey == "min_edge_length")
    {
        switch (ct)
        {
        case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14:
            return true;
        default:
            return false;
        }
    }
    return false;
}

bool finiteRange(vtkDataSet* ds, vtkDataArray* arr, const QString& metricKey, double outRange[2])
{
    if (!ds || !arr)
        return false;
    bool inited = false;
    const vtkIdType n = ds->GetNumberOfCells();
    for (vtkIdType i = 0; i < n; ++i)
    {
        const int ct = ds->GetCellType(i);
        if (!vtklasttry::isMetricSupportedCell(metricKey, ct))
            continue;
        const double v = arr->GetTuple1(i);
        if (!vtklasttry::isFinite(v))
            continue;
        if (!inited)
        {
            outRange[0] = v;
            outRange[1] = v;
            inited = true;
        }
        else
        {
            if (v < outRange[0]) outRange[0] = v;
            if (v > outRange[1]) outRange[1] = v;
        }
    }
    return inited;
}
} // namespace vtklasttry

