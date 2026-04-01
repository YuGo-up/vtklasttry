#include "QualityDefaults.h"

namespace vtklasttry
{
QualityDefaultsRecommendation recommendQualityDefaults(const QString& metricKey,
                                                      double currentThreshold,
                                                      QualityDefaultsContext ctx)
{
    QualityDefaultsRecommendation r;

    // Context-specific defaults used in current UI.
    if (ctx == QualityDefaultsContext::MinEdgeLength)
    {
        r.badRuleKey = QStringLiteral("le");
        if (currentThreshold <= 0.0)
        {
            r.hasThreshold = true;
            r.threshold = 1e-6;
        }
        return r;
    }
    if (ctx == QualityDefaultsContext::NonOrtho)
    {
        r.badRuleKey = QStringLiteral("ge");
        if (currentThreshold <= 0.0 || currentThreshold > 180.0)
        {
            r.hasThreshold = true;
            r.threshold = 70.0;
        }
        return r;
    }

    // Generic (vtkMeshQuality) path: threshold corrections and default bad-rule direction.
    // Keep behavior consistent with previous inlined logic.
    if (metricKey == "skew" && currentThreshold > 1.0)
    {
        r.hasThreshold = true;
        r.threshold = 0.5;
    }

    // Default bad-rule direction by metric.
    if (metricKey == "aspect" || metricKey == "skew" || metricKey == "non_ortho" || metricKey == "jacobian_ratio" || metricKey == "max_angle")
        r.badRuleKey = QStringLiteral("ge");
    else if (metricKey == "scaled_jacobian" || metricKey == "jacobian" || metricKey == "min_angle" || metricKey == "min_edge_length")
        r.badRuleKey = QStringLiteral("le");

    // Metric-specific threshold adjustments.
    if (metricKey == "scaled_jacobian" && currentThreshold > 1.0)
    {
        r.hasThreshold = true;
        r.threshold = 0.2;
    }
    if (metricKey == "jacobian" && currentThreshold > 1.0)
    {
        r.hasThreshold = true;
        r.threshold = 0.05;
    }
    if (metricKey == "min_angle" && currentThreshold > 180.0)
    {
        r.hasThreshold = true;
        r.threshold = 25.0;
    }
    if (metricKey == "min_edge_length" && currentThreshold <= 0.0)
    {
        r.hasThreshold = true;
        r.threshold = 1e-6;
    }
    if (metricKey == "max_angle" && currentThreshold < 90.0)
    {
        r.hasThreshold = true;
        r.threshold = 160.0;
    }
    if (metricKey == "jacobian_ratio" && currentThreshold < 1.0)
    {
        r.hasThreshold = true;
        r.threshold = 10.0;
    }

    return r;
}
} // namespace vtklasttry

