#include "MeshQualityCalculator.h"

#include <cmath>

#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkMath.h>
#include <vtkPolyData.h>

namespace
{
    double edgeLength(const double p0[3], const double p1[3])
    {
        return std::sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
    }

    // 简单三角形长宽比：最大边 / 最小边
    double triangleAspectRatio(const double p0[3],
                               const double p1[3],
                               const double p2[3])
    {
        const double l0 = edgeLength(p0, p1);
        const double l1 = edgeLength(p1, p2);
        const double l2 = edgeLength(p2, p0);

        const double maxL = std::max(l0, std::max(l1, l2));
        const double minL = std::min(l0, std::min(l1, l2));

        if (minL <= 0.0)
        {
            return std::numeric_limits<double>::infinity();
        }

        return maxL / minL;
    }

    // Simple triangle skewness based on deviation of max interior angle
    double triangleSkewness(const double p0[3],
                            const double p1[3],
                            const double p2[3])
    {
        // Compute three interior angles manually
        auto angleAt = [](const double a[3], const double b[3], const double c[3]) -> double
        {
            double v1[3], v2[3];
            v1[0] = a[0] - b[0];
            v1[1] = a[1] - b[1];
            v1[2] = a[2] - b[2];
            v2[0] = c[0] - b[0];
            v2[1] = c[1] - b[1];
            v2[2] = c[2] - b[2];

            vtkMath::Normalize(v1);
            vtkMath::Normalize(v2);
            double dot = vtkMath::Dot(v1, v2);
            if (dot > 1.0) dot = 1.0;
            if (dot < -1.0) dot = -1.0;
            return vtkMath::DegreesFromRadians(std::acos(dot));
        };

        double a0 = angleAt(p1, p0, p2);
        double a1 = angleAt(p0, p1, p2);
        double a2 = angleAt(p0, p2, p1);

        double maxAngleDeg = std::max(a0, std::max(a1, a2));

        const double ideal = 60.0;
        const double maxPossible = 180.0;
        double skew = (maxAngleDeg - ideal) / (maxPossible - ideal); // (maxAngle-60)/120
        if (skew < 0.0) skew = 0.0;
        if (skew > 1.0) skew = 1.0;
        return skew;
    }
}

bool MeshQualityCalculator::computeQuality(vtkSmartPointer<vtkPolyData> polyData,
                                           QualityData& out,
                                           double aspectThreshold,
                                           double skewThreshold)
{
    if (!polyData)
    {
        return false;
    }

    const vtkIdType numCells = polyData->GetNumberOfCells();
    if (numCells <= 0)
    {
        return false;
    }

    out.aspectRatio.assign(static_cast<std::size_t>(numCells), 0.0);
    out.skewness.assign(static_cast<std::size_t>(numCells), 0.0);
    out.badCells.clear();
    out.aspectMin = std::numeric_limits<double>::max();
    out.aspectMax = std::numeric_limits<double>::lowest();
    out.skewMin = std::numeric_limits<double>::max();
    out.skewMax = std::numeric_limits<double>::lowest();

    vtkSmartPointer<vtkDoubleArray> aspectArray =
        vtkSmartPointer<vtkDoubleArray>::New();
    aspectArray->SetName("AspectRatio");
    aspectArray->SetNumberOfComponents(1);
    aspectArray->SetNumberOfTuples(numCells);

    vtkSmartPointer<vtkDoubleArray> skewArray =
        vtkSmartPointer<vtkDoubleArray>::New();
    skewArray->SetName("Skewness");
    skewArray->SetNumberOfComponents(1);
    skewArray->SetNumberOfTuples(numCells);

    double p0[3], p1[3], p2[3];

    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
        vtkCell* cell = polyData->GetCell(cellId);
        if (!cell || cell->GetNumberOfPoints() < 3)
        {
            // 非三角形或异常单元，标记为 0
            aspectArray->SetTuple1(cellId, 0.0);
            skewArray->SetTuple1(cellId, 0.0);
            continue;
        }

        // 这里假定 STL 网格主要是三角形
        vtkIdType id0 = cell->GetPointId(0);
        vtkIdType id1 = cell->GetPointId(1);
        vtkIdType id2 = cell->GetPointId(2);

        polyData->GetPoint(id0, p0);
        polyData->GetPoint(id1, p1);
        polyData->GetPoint(id2, p2);

        double aspect = triangleAspectRatio(p0, p1, p2);
        double skew = triangleSkewness(p0, p1, p2);

        out.aspectRatio[static_cast<std::size_t>(cellId)] = aspect;
        out.skewness[static_cast<std::size_t>(cellId)] = skew;

        aspectArray->SetTuple1(cellId, aspect);
        skewArray->SetTuple1(cellId, skew);

        updateStatsAndBadCells(out, aspect, skew, cellId, aspectThreshold, skewThreshold);
    }

    vtkCellData* cellData = polyData->GetCellData();
    if (cellData)
    {
        cellData->AddArray(aspectArray);
        cellData->AddArray(skewArray);
        cellData->SetScalars(aspectArray); // default active scalar: aspect ratio

        vtkSmartPointer<vtkDoubleArray> badArray = vtkSmartPointer<vtkDoubleArray>::New();
        badArray->SetName("BadCell");
        badArray->SetNumberOfComponents(1);
        badArray->SetNumberOfTuples(numCells);
        for (vtkIdType i = 0; i < numCells; ++i)
            badArray->SetTuple1(i, 0.0);
        for (vtkIdType id : out.badCells)
            badArray->SetTuple1(id, 1.0);
        cellData->AddArray(badArray);
    }

    return true;
}

void MeshQualityCalculator::updateStatsAndBadCells(QualityData& data,
                                                   double aspectValue,
                                                   double skewValue,
                                                   vtkIdType cellId,
                                                   double aspectThreshold,
                                                   double skewThreshold)
{
    if (aspectValue > 0.0 && std::isfinite(aspectValue))
    {
        data.aspectMin = std::min(data.aspectMin, aspectValue);
        data.aspectMax = std::max(data.aspectMax, aspectValue);
    }
    if (skewValue >= 0.0 && std::isfinite(skewValue))
    {
        data.skewMin = std::min(data.skewMin, skewValue);
        data.skewMax = std::max(data.skewMax, skewValue);
    }

    if ((aspectThreshold > 0.0 && aspectValue > aspectThreshold) ||
        (skewThreshold > 0.0 && skewValue > skewThreshold))
    {
        data.badCells.push_back(cellId);
    }
}

