#pragma once

#include <QString>

namespace vtklasttry
{
struct QualityDefaultsRecommendation
{
    QString badRuleKey;     // "ge"/"le" or empty (no change)
    bool hasThreshold = false;
    double threshold = 0.0;
};

enum class QualityDefaultsContext
{
    Generic,        // vtkMeshQuality path
    MinEdgeLength,  // custom metric min_edge_length
    NonOrtho        // custom metric non_ortho
};

// Pure: does not touch UI; caller applies to widgets.
QualityDefaultsRecommendation recommendQualityDefaults(const QString& metricKey,
                                                      double currentThreshold,
                                                      QualityDefaultsContext ctx);
} // namespace vtklasttry

