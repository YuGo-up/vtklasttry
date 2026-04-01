#include "QualityEngine.h"

#include <cmath>
#include <limits>
#include <vector>

#include <vtkCell.h>
#include <vtkCell3D.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkMeshQuality.h>
#include <vtkNew.h>
#include <vtkUnstructuredGrid.h>

#include "QualitySupport.h"

namespace
{
// isFinite()/isMetricSupportedCell() moved to QualitySupport.*
static double dot3(const double a[3], const double b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
static void sub3(const double a[3], const double b[3], double out[3]) { out[0] = a[0] - b[0]; out[1] = a[1] - b[1]; out[2] = a[2] - b[2]; }
static double norm3(const double v[3]) { return std::sqrt(dot3(v, v)); }
static bool normalize3(double v[3])
{
    const double n = norm3(v);
    if (n <= 0.0 || !vtklasttry::isFinite(n))
        return false;
    v[0] /= n;
    v[1] /= n;
    v[2] /= n;
    return true;
}

static bool faceNormal(vtkCell* face, double outN[3])
{
    if (!face || face->GetNumberOfPoints() < 3)
        return false;
    double n[3] = { 0, 0, 0 };
    const vtkIdType m = face->GetNumberOfPoints();
    double p0[3], p1[3];
    for (vtkIdType i = 0; i < m; ++i)
    {
        face->GetPoints()->GetPoint(i, p0);
        face->GetPoints()->GetPoint((i + 1) % m, p1);
        n[0] += (p0[1] - p1[1]) * (p0[2] + p1[2]);
        n[1] += (p0[2] - p1[2]) * (p0[0] + p1[0]);
        n[2] += (p0[0] - p1[0]) * (p0[1] + p1[1]);
    }
    outN[0] = n[0];
    outN[1] = n[1];
    outN[2] = n[2];
    return normalize3(outN);
}

static void cellCenter(vtkCell* cell, double outC[3])
{
    outC[0] = outC[1] = outC[2] = 0.0;
    if (!cell || cell->GetNumberOfPoints() <= 0)
        return;
    const vtkIdType n = cell->GetNumberOfPoints();
    double p[3];
    for (vtkIdType i = 0; i < n; ++i)
    {
        cell->GetPoints()->GetPoint(i, p);
        outC[0] += p[0];
        outC[1] += p[1];
        outC[2] += p[2];
    }
    outC[0] /= static_cast<double>(n);
    outC[1] /= static_cast<double>(n);
    outC[2] /= static_cast<double>(n);
}

static double minEdgeLengthForCell(vtkDataSet* ds, vtkIdType cellId, const QString& metricKey)
{
    (void)metricKey;
    if (!ds)
        return std::numeric_limits<double>::quiet_NaN();
    vtkCell* cell = ds->GetCell(cellId);
    if (!cell)
        return std::numeric_limits<double>::quiet_NaN();
    const int ne = cell->GetNumberOfEdges();
    if (ne <= 0)
        return std::numeric_limits<double>::quiet_NaN();
    double minLen = std::numeric_limits<double>::infinity();
    for (int e = 0; e < ne; ++e)
    {
        vtkCell* edge = cell->GetEdge(e);
        if (!edge || edge->GetNumberOfPoints() < 2)
            continue;
        double p0[3], p1[3];
        edge->GetPoints()->GetPoint(0, p0);
        edge->GetPoints()->GetPoint(1, p1);
        const double dx = p1[0] - p0[0], dy = p1[1] - p0[1], dz = p1[2] - p0[2];
        const double len = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (vtklasttry::isFinite(len) && len < minLen)
            minLen = len;
    }
    if (!vtklasttry::isFinite(minLen) || minLen >= std::numeric_limits<double>::infinity() / 2)
        return std::numeric_limits<double>::quiet_NaN();
    return minLen;
}

static bool canceled(const std::atomic<bool>* f) { return f && f->load(); }
} // namespace

namespace vtklasttry
{
vtkSmartPointer<vtkDataArray> computeMinEdgeLengthQualityArray(vtkDataSet* ds, const std::atomic<bool>* cancelFlag)
{
    if (!ds)
        return nullptr;
    vtkNew<vtkDoubleArray> arr;
    arr->SetNumberOfComponents(1);
    arr->SetNumberOfTuples(ds->GetNumberOfCells());
    arr->SetName("Quality");
    const double nanv = std::numeric_limits<double>::quiet_NaN();
    const QString metricKey = QStringLiteral("min_edge_length");
    for (vtkIdType i = 0; i < ds->GetNumberOfCells(); ++i)
    {
        if ((i % 4096) == 0 && canceled(cancelFlag))
            return nullptr;
        const int ct = ds->GetCellType(i);
        if (!vtklasttry::isMetricSupportedCell(metricKey, ct))
        {
            arr->SetTuple1(i, nanv);
            continue;
        }
        arr->SetTuple1(i, minEdgeLengthForCell(ds, i, metricKey));
    }
    return vtkSmartPointer<vtkDataArray>(arr.GetPointer());
}

vtkSmartPointer<vtkDataArray> computeNonOrthoQualityArray(vtkDataSet* ds, const std::atomic<bool>* cancelFlag)
{
    if (!ds)
        return nullptr;

    vtkNew<vtkDoubleArray> arr;
    arr->SetNumberOfComponents(1);
    arr->SetNumberOfTuples(ds->GetNumberOfCells());
    arr->SetName("Quality");

    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(ds);
    if (ug)
        ug->BuildLinks();

    std::vector<double> centers;
    centers.resize(static_cast<size_t>(ds->GetNumberOfCells()) * 3, 0.0);
    for (vtkIdType i = 0; i < ds->GetNumberOfCells(); ++i)
    {
        if ((i % 1024) == 0 && canceled(cancelFlag))
            return nullptr;
        double c[3];
        cellCenter(ds->GetCell(i), c);
        const size_t base = static_cast<size_t>(i) * 3;
        centers[base + 0] = c[0];
        centers[base + 1] = c[1];
        centers[base + 2] = c[2];
    }

    const double nanv = std::numeric_limits<double>::quiet_NaN();
    vtkNew<vtkIdList> facePtIds;
    vtkNew<vtkIdList> neiIds;
    for (vtkIdType i = 0; i < ds->GetNumberOfCells(); ++i)
    {
        if ((i % 128) == 0 && canceled(cancelFlag))
            return nullptr;
        const int ct = ds->GetCellType(i);
        if (!vtklasttry::isMetricSupportedCell(QStringLiteral("non_ortho"), ct))
        {
            arr->SetTuple1(i, nanv);
            continue;
        }
        vtkCell* cell = ds->GetCell(i);
        vtkCell3D* c3d = vtkCell3D::SafeDownCast(cell);
        if (!c3d || c3d->GetNumberOfFaces() <= 0)
        {
            arr->SetTuple1(i, nanv);
            continue;
        }
        const size_t cbase = static_cast<size_t>(i) * 3;
        const double cc[3] = { centers[cbase + 0], centers[cbase + 1], centers[cbase + 2] };

        double worst = 0.0;
        bool any = false;
        const int nFaces = c3d->GetNumberOfFaces();
        for (int f = 0; f < nFaces; ++f)
        {
            vtkCell* face = c3d->GetFace(f);
            if (!face || face->GetNumberOfPoints() < 3)
                continue;

            double n[3];
            if (!faceNormal(face, n))
                continue;

            facePtIds->Reset();
            for (vtkIdType k = 0; k < face->GetNumberOfPoints(); ++k)
                facePtIds->InsertNextId(face->GetPointId(k));

            neiIds->Reset();
            ds->GetCellNeighbors(i, facePtIds, neiIds);
            if (neiIds->GetNumberOfIds() <= 0)
                continue;

            const vtkIdType neiId = neiIds->GetId(0);
            const size_t nbase = static_cast<size_t>(neiId) * 3;
            const double nc[3] = { centers[nbase + 0], centers[nbase + 1], centers[nbase + 2] };
            double d[3];
            sub3(nc, cc, d);
            if (!normalize3(d))
                continue;

            double c = std::fabs(dot3(n, d));
            if (c > 1.0)
                c = 1.0;
            const double theta = std::acos(c) * 180.0 / 3.14159265358979323846;
            if (!any)
            {
                worst = theta;
                any = true;
            }
            else if (theta > worst)
                worst = theta;
        }
        const double v = any ? worst : nanv;
        arr->SetTuple1(i, v);
    }

    return vtkSmartPointer<vtkDataArray>(arr.GetPointer());
}

int countSupportedCells(vtkDataSet* ds, const QString& metricKey)
{
    if (!ds)
        return 0;
    int cnt = 0;
    const vtkIdType n = ds->GetNumberOfCells();
    for (vtkIdType i = 0; i < n; ++i)
    {
        if (vtklasttry::isMetricSupportedCell(metricKey, ds->GetCellType(i)))
            ++cnt;
    }
    return cnt;
}

ComputeResult computeQualityOnCopy(vtkDataSet* source, const QStringList& metricKeys, const std::atomic<bool>* cancelFlag)
{
    ComputeResult r;
    if (!source)
        return r;
    if (canceled(cancelFlag))
    {
        r.canceled = true;
        return r;
    }

    vtkSmartPointer<vtkDataSet> ds = vtkSmartPointer<vtkDataSet>::Take(source->NewInstance());
    ds->DeepCopy(source);

    QHash<QString, vtkSmartPointer<vtkDataArray>> cache;

    auto addQualityArray = [&](vtkDataArray* arrRaw, const QString& metricKey) -> vtkSmartPointer<vtkDataArray> {
        if (!arrRaw)
            return nullptr;
        vtkSmartPointer<vtkDataArray> arr;
        arr.TakeReference(arrRaw->NewInstance());
        arr->DeepCopy(arrRaw);
        const QString name = QString("Quality_%1").arg(metricKey);
        arr->SetName(name.toLocal8Bit().constData());
        ds->GetCellData()->AddArray(arr);
        return arr;
    };

    for (const QString& metricKey : metricKeys)
    {
        if (canceled(cancelFlag))
        {
            r.canceled = true;
            return r;
        }

        const int supportedN = countSupportedCells(ds, metricKey);
        if (supportedN <= 0)
            return r;

        vtkSmartPointer<vtkDataArray> qualityArr;

        if (metricKey == "min_edge_length")
        {
            vtkSmartPointer<vtkDataArray> arr = vtklasttry::computeMinEdgeLengthQualityArray(ds, cancelFlag);
            if (!arr)
            {
                r.canceled = true;
                return r;
            }
            qualityArr = addQualityArray(arr, metricKey);
        }
        else if (metricKey == "non_ortho")
        {
            vtkSmartPointer<vtkDataArray> arr = vtklasttry::computeNonOrthoQualityArray(ds, cancelFlag);
            if (!arr)
            {
                r.canceled = true;
                return r;
            }
            qualityArr = addQualityArray(arr, metricKey);
        }
        else
        {
            vtkNew<vtkMeshQuality> q;
            q->SetInputData(ds);
            q->SetSaveCellQuality(true);

            if (metricKey == "aspect")
            {
                q->SetTriangleQualityMeasureToAspectRatio();
                q->SetQuadQualityMeasureToAspectRatio();
                q->SetTetQualityMeasureToAspectRatio();
                q->SetHexQualityMeasureToEdgeRatio();
            }
            else if (metricKey == "skew")
            {
                q->SetTriangleQualityMeasureToShapeAndSize();
                q->SetQuadQualityMeasureToSkew();
                q->SetTetQualityMeasureToScaledJacobian();
                q->SetHexQualityMeasureToSkew();
            }
            else if (metricKey == "scaled_jacobian")
            {
                q->SetTriangleQualityMeasureToScaledJacobian();
                q->SetQuadQualityMeasureToScaledJacobian();
                q->SetTetQualityMeasureToScaledJacobian();
                q->SetHexQualityMeasureToScaledJacobian();
            }
            else if (metricKey == "jacobian")
            {
                q->SetTriangleQualityMeasureToScaledJacobian();
                q->SetQuadQualityMeasureToJacobian();
                q->SetTetQualityMeasureToJacobian();
                q->SetHexQualityMeasureToJacobian();
            }
            else if (metricKey == "jacobian_ratio")
            {
                q->SetTriangleQualityMeasureToEdgeRatio();
                q->SetQuadQualityMeasureToEdgeRatio();
                q->SetTetQualityMeasureToEdgeRatio();
                q->SetHexQualityMeasureToEdgeRatio();
            }
            else if (metricKey == "min_angle")
            {
                q->SetTriangleQualityMeasureToMinAngle();
                q->SetQuadQualityMeasureToMinAngle();
                q->SetTetQualityMeasureToMinAngle();
            }
            else if (metricKey == "max_angle")
            {
                q->SetTriangleQualityMeasureToMaxAngle();
                q->SetQuadQualityMeasureToMaxAngle();
            }
            else
            {
                return r;
            }

            q->Update();
            if (canceled(cancelFlag))
            {
                r.canceled = true;
                return r;
            }

            vtkDataSet* outRaw = q->GetOutput();
            vtkDataArray* qraw = (outRaw && outRaw->GetCellData()) ? outRaw->GetCellData()->GetArray("Quality") : nullptr;
            qualityArr = addQualityArray(qraw, metricKey);
        }

        if (!qualityArr)
            return r;

        const vtkIdType nCells = ds->GetNumberOfCells();
        const double nanv = std::numeric_limits<double>::quiet_NaN();
        for (vtkIdType i = 0; i < nCells; ++i)
        {
            if ((i % 4096) == 0 && canceled(cancelFlag))
            {
                r.canceled = true;
                return r;
            }
            const int ct = ds->GetCellType(i);
            if (!vtklasttry::isMetricSupportedCell(metricKey, ct))
                qualityArr->SetTuple1(i, nanv);
        }

        cache.insert(metricKey, qualityArr);
    }

    r.ok = true;
    r.meshData = ds;
    r.qualityCache = cache;
    return r;
}
} // namespace vtklasttry

