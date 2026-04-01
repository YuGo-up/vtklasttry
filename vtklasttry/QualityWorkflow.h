#pragma once

#include "QualityRequest.h"
#include "QualityState.h"

class QStatusBar;
class QTextEdit;
class QWidget;

#include <functional>

namespace vtklasttry
{
class QualityWorkflow
{
public:
    static bool computeQuality(QWidget* parent,
                               QTextEdit* logBox,
                               QStatusBar* statusBar,
                               const vtklasttry::QualityRequest& req,
                               vtklasttry::QualityState& state,
                               const std::function<void(const QString&)>& setBadRuleKey,
                               const std::function<void(double)>& setThreshold,
                               const std::function<void()>& applyThresholdTemplateIfNeeded);
};
} // namespace vtklasttry

