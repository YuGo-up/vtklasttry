#include "ThresholdTemplates.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <vtkDataArray.h>
#include <vtkDataSet.h>

#include "QualityEngine.h"
#include "QualitySupport.h"

namespace
{
static double adaptiveQuantile(vtkDataSet* mesh, vtkDataArray* qa, const QString& metricKey, double q)
{
    if (!mesh || !qa)
        return std::numeric_limits<double>::quiet_NaN();
    const vtkIdType n = mesh->GetNumberOfCells();
    if (n <= 0)
        return std::numeric_limits<double>::quiet_NaN();
    std::vector<double> vals;
    vals.reserve(static_cast<size_t>(n));
    for (vtkIdType i = 0; i < n; ++i)
    {
        const int ct = mesh->GetCellType(i);
        if (!vtklasttry::isMetricSupportedCell(metricKey, ct))
            continue;
        const double v = qa->GetTuple1(i);
        if (!vtklasttry::isFinite(v))
            continue;
        vals.push_back(v);
    }
    if (vals.empty())
        return std::numeric_limits<double>::quiet_NaN();
    q = std::max(0.0, std::min(1.0, q));
    const size_t k = static_cast<size_t>(q * (vals.size() - 1));
    std::nth_element(vals.begin(), vals.begin() + k, vals.end());
    return vals[k];
}
} // namespace

namespace vtklasttry
{
TemplateApplyOutput applyThresholdTemplate(const TemplateApplyInput& in)
{
    TemplateApplyOutput out;
    if (in.templateKey.isEmpty() || in.metricKey.isEmpty())
        return out;

    const QString& tpl = in.templateKey;
    const QString& metricKey = in.metricKey;

    auto aq = [&](double q) { return adaptiveQuantile(in.mesh, in.qualityArray, metricKey, q); };

    if (metricKey == "aspect")
    {
        out.ruleKey = "ge";
        double thr = 5.0;
        if (tpl == "std_strict") thr = 3.0;
        else if (tpl == "std_normal") thr = 5.0;
        else if (tpl == "std_loose") thr = 10.0;
        else if (tpl == "adaptive") thr = aq(0.95);
        out.threshold = thr;
        out.ok = true;
        return out;
    }
    if (metricKey == "skew")
    {
        out.ruleKey = "ge";
        double thr = 0.5;
        if (tpl == "std_strict") thr = 0.3;
        else if (tpl == "std_normal") thr = 0.5;
        else if (tpl == "std_loose") thr = 0.8;
        else if (tpl == "adaptive") thr = aq(0.95);
        out.threshold = thr;
        out.ok = true;
        return out;
    }
    if (metricKey == "scaled_jacobian")
    {
        out.ruleKey = "le";
        double thr = 0.2;
        if (tpl == "std_strict") thr = 0.3;
        else if (tpl == "std_normal") thr = 0.2;
        else if (tpl == "std_loose") thr = 0.1;
        else if (tpl == "adaptive") thr = aq(0.05);
        out.threshold = thr;
        out.ok = true;
        return out;
    }
    if (metricKey == "jacobian")
    {
        out.ruleKey = "le";
        double thr = 0.05;
        if (tpl == "std_strict") thr = 0.10;
        else if (tpl == "std_normal") thr = 0.05;
        else if (tpl == "std_loose") thr = 0.01;
        else if (tpl == "adaptive") thr = aq(0.05);
        out.threshold = thr;
        out.ok = true;
        return out;
    }
    if (metricKey == "jacobian_ratio")
    {
        out.ruleKey = "ge";
        double thr = 10.0;
        if (tpl == "std_strict") thr = 5.0;
        else if (tpl == "std_normal") thr = 10.0;
        else if (tpl == "std_loose") thr = 20.0;
        else if (tpl == "adaptive") thr = aq(0.95);
        out.threshold = thr;
        out.ok = true;
        return out;
    }
    if (metricKey == "min_angle")
    {
        out.ruleKey = "le";
        double thr = 25.0;
        if (tpl == "std_strict") thr = 30.0;
        else if (tpl == "std_normal") thr = 25.0;
        else if (tpl == "std_loose") thr = 15.0;
        else if (tpl == "adaptive") thr = aq(0.05);
        out.threshold = thr;
        out.ok = true;
        return out;
    }
    if (metricKey == "max_angle")
    {
        out.ruleKey = "ge";
        double thr = 160.0;
        if (tpl == "std_strict") thr = 150.0;
        else if (tpl == "std_normal") thr = 160.0;
        else if (tpl == "std_loose") thr = 170.0;
        else if (tpl == "adaptive") thr = aq(0.95);
        out.threshold = thr;
        out.ok = true;
        return out;
    }
    if (metricKey == "non_ortho")
    {
        out.ruleKey = "ge";
        double thr = 70.0;
        if (tpl == "std_strict") thr = 60.0;
        else if (tpl == "std_normal") thr = 70.0;
        else if (tpl == "std_loose") thr = 80.0;
        else if (tpl == "adaptive") thr = aq(0.95);
        out.threshold = thr;
        out.ok = true;
        return out;
    }
    if (metricKey == "min_edge_length")
    {
        out.ruleKey = "le";
        double thr = 1e-6;
        if (tpl == "std_strict") thr = 1e-5;
        else if (tpl == "std_normal") thr = 1e-6;
        else if (tpl == "std_loose") thr = 1e-7;
        else if (tpl == "adaptive") thr = aq(0.01);
        out.threshold = thr;
        out.ok = true;
        return out;
    }

    return out;
}
} // namespace vtklasttry

