#pragma once

#include "RuleEngine.h"

#include <QString>
#include <QVector>

namespace vtklasttry
{
struct ComputeRequest
{
    QString primaryMetricKey;
    bool comboEnabled = false;
    QString logicOp; // "and"/"or" (reserved; may be used later)
    QVector<vtklasttry::RuleCondition> tableConds; // used to derive required metric keys in combo mode
};
} // namespace vtklasttry

