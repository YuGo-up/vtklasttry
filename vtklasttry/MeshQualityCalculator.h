#pragma once

#include <vector>
#include <limits>
#include <string>

#include <vtkSmartPointer.h>
#include <vtkIdTypeArray.h>

class vtkPolyData;

struct QualityData
{
    std::vector<double> aspectRatio;
    std::vector<double> skewness;
    std::vector<vtkIdType> badCells;

    double aspectMin = std::numeric_limits<double>::max();
    double aspectMax = std::numeric_limits<double>::lowest();
    double skewMin = std::numeric_limits<double>::max();
    double skewMax = std::numeric_limits<double>::lowest();
};

// 负责基于 vtkPolyData 计算网格质量指标，并把结果写回到 polyData 的 cell data 中
class MeshQualityCalculator
{
public:
    MeshQualityCalculator() = default;

    // thresholds 仅用于识别低质量单元
    bool computeQuality(vtkSmartPointer<vtkPolyData> polyData,
                        QualityData& out,
                        double aspectThreshold,
                        double skewThreshold);

private:
    void updateStatsAndBadCells(QualityData& data,
                                double aspectValue,
                                double skewValue,
                                vtkIdType cellId,
                                double aspectThreshold,
                                double skewThreshold);
};

