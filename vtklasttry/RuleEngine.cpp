#include "RuleEngine.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTableWidget>

#include "QualityEngine.h"

#include <cmath>
#include <limits>

#include <vtkDataArray.h>
#include <vtkDataSet.h>

#include "QualitySupport.h"

namespace vtklasttry
{
static bool isFinite(double v) { return std::isfinite(v) != 0; }

QVector<RuleCondition> collectRuleConditionsFromTable(QTableWidget* table)
{
    QVector<RuleCondition> conds;
    if (!table)
        return conds;
    const int rows = table->rowCount();
    conds.reserve(rows);
    for (int r = 0; r < rows; ++r)
    {
        auto* cbMetric = qobject_cast<QComboBox*>(table->cellWidget(r, 0));
        auto* cbRule = qobject_cast<QComboBox*>(table->cellWidget(r, 1));
        auto* spThr = qobject_cast<QDoubleSpinBox*>(table->cellWidget(r, 2));
        auto* cbTpl = qobject_cast<QComboBox*>(table->cellWidget(r, 3));
        if (!cbMetric || !cbRule || !spThr)
            continue;

        RuleCondition c;
        c.metricKey = cbMetric->currentData().toString();
        c.isGE = (cbRule->currentData().toString() == "ge");
        c.threshold = spThr->value();
        if (cbTpl)
            c.templateKey = cbTpl->currentData().toString();

        if (c.metricKey.isEmpty())
            continue;
        conds.push_back(c);
    }
    return conds;
}

QString formatComboRuleText(const QVector<RuleCondition>& conds, const QString& logicOp)
{
    if (conds.isEmpty())
        return QStringLiteral("-");
    const QString op = (logicOp == "or") ? QStringLiteral(" OR ") : QStringLiteral(" AND ");
    QStringList parts;
    for (const RuleCondition& c : conds)
    {
        parts << QString("(%1 q %2 %3)")
                     .arg(c.metricKey)
                     .arg(c.isGE ? ">=" : "<=")
                     .arg(c.threshold, 0, 'g', 8);
    }
    return parts.join(op);
}

BadCellEvalResult evaluateBadCellsSingle(vtkDataSet* mesh,
                                        vtkDataArray* qualityArray,
                                        const QString& metricKey,
                                        bool isGE,
                                        double threshold)
{
    BadCellEvalResult r;
    if (!mesh || !qualityArray)
    {
        r.error = QString::fromWCharArray(L"\u7F51\u683C/\u8D28\u91CF\u6570\u7EC4\u4E3A\u7A7A\u3002");
        return r;
    }
    r.totalCells = mesh->GetNumberOfCells();
    r.badIdsUnion.reserve(static_cast<int>(r.totalCells));
    for (vtkIdType i = 0; i < r.totalCells; ++i)
    {
        const int ct = mesh->GetCellType(i);
        if (!vtklasttry::isMetricSupportedCell(metricKey, ct))
            continue;
        const double v = qualityArray->GetTuple1(i);
        if (!vtklasttry::isFinite(v))
            continue;
        const bool bad = (isGE && v >= threshold) || (!isGE && v <= threshold);
        if (bad)
            r.badIdsUnion.push_back(i);
    }
    r.ok = true;
    return r;
}

BadCellEvalResult evaluateBadCellsCombo(vtkDataSet* mesh,
                                       const QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCache,
                                       const QVector<RuleCondition>& rules,
                                       const QString& logicOp)
{
    BadCellEvalResult r;
    if (!mesh)
    {
        r.error = QString::fromWCharArray(L"\u7F51\u683C\u4E3A\u7A7A\u3002");
        return r;
    }
    r.totalCells = mesh->GetNumberOfCells();
    const bool isAnd = (logicOp != "or");

    r.badIdsUnion.clear();
    r.badIdsUnion.reserve(static_cast<int>(r.totalCells));
    r.badIdsByRule.clear();
    r.badIdsByRule.resize(rules.size());
    r.supportedByRule.clear();
    r.supportedByRule.resize(rules.size());

    // Per-rule scan (for legend + overlay) and union scan (for list/export).
    // We do both in one pass over cells for efficiency.
    for (vtkIdType cellId = 0; cellId < r.totalCells; ++cellId)
    {
        const int ct = mesh->GetCellType(cellId);

        bool acc = isAnd;
        bool hasAny = false;

        for (int ri = 0; ri < rules.size(); ++ri)
        {
            const RuleCondition& c = rules[ri];
            if (!vtklasttry::isMetricSupportedCell(c.metricKey, ct))
            {
                if (isAnd)
                {
                    acc = false;
                    // AND: unsupported breaks whole conjunction
                    // Still continue counting supportedByRule? no.
                    break;
                }
                continue;
            }

            r.supportedByRule[ri] += 1;

            vtkDataArray* qa = qualityCache.value(c.metricKey);
            if (!qa)
            {
                if (isAnd)
                {
                    acc = false;
                    break;
                }
                continue;
            }

            const double v = qa->GetTuple1(cellId);
            if (!vtklasttry::isFinite(v))
            {
                if (isAnd)
                {
                    acc = false;
                    break;
                }
                continue;
            }

            hasAny = true;
            const bool bi = (c.isGE && v >= c.threshold) || (!c.isGE && v <= c.threshold);
            if (bi)
                r.badIdsByRule[ri].push_back(cellId);

            if (isAnd)
                acc = acc && bi;
            else
                acc = acc || bi;
        }

        if (hasAny && acc)
            r.badIdsUnion.push_back(cellId);
    }

    r.ok = true;
    return r;
}
} // namespace vtklasttry

