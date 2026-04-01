#pragma once

#include <QString>

namespace vtklasttry
{
struct QualityRequest
{
    QString metricKey;
    double thresholdValue = 0.0;
    QString templateKey;
};
} // namespace vtklasttry

