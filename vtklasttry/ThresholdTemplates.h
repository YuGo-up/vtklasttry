#pragma once

#include <QString>

class vtkDataArray;
class vtkDataSet;

namespace vtklasttry
{
struct TemplateApplyInput
{
    QString templateKey; // std_strict/std_normal/std_loose/adaptive
    QString metricKey;
    vtkDataSet* mesh = nullptr;
    vtkDataArray* qualityArray = nullptr; // array for metricKey (may contain NaN for unsupported)
};

struct TemplateApplyOutput
{
    bool ok = false;
    QString ruleKey; // "ge"/"le"
    double threshold = 0.0;
};

// Decide (ruleKey, threshold) for a template+metric on current mesh.
// Pure computation: does not touch Qt widgets.
TemplateApplyOutput applyThresholdTemplate(const TemplateApplyInput& in);
} // namespace vtklasttry

