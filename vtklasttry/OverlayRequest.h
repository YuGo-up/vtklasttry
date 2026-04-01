#pragma once

#include "RuleEngine.h"

#include <QPair>
#include <QString>
#include <QVector>

namespace vtklasttry
{
struct OverlayRequest
{
    bool showBad = true;

    // Single metric rule (when comboEnabled == false)
    QString metricKeySingle;
    bool isGESingle = true;
    double thresholdSingle = 0.0;

    // Combo rule (when comboEnabled == true)
    bool comboEnabled = false;
    QString logicOp; // "and"/"or"
    QVector<vtklasttry::RuleCondition> tableConds;
    QString comboRuleText; // preformatted, avoids SceneController reading UI

    // UI metric items (key -> display text), used for legend naming
    QVector<QPair<QString, QString>> metricUiItems;
};
} // namespace vtklasttry

