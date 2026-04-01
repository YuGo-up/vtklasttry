#pragma once

#include <QString>
#include <QVector>
#include <QHash>

class QTableWidget;
class vtkDataArray;
class vtkDataSet;
template <typename T> class vtkSmartPointer;
using vtkIdType = long long;

namespace vtklasttry
{
struct RuleCondition
{
    QString metricKey;
    bool isGE = true; // true: >=, false: <=
    double threshold = 0.0;
    QString templateKey; // optional
};

struct BadCellEvalResult
{
    bool ok = false;
    QString error;
    vtkIdType totalCells = 0;

    // Union bad cell ids (0..N-1) in dataset index space.
    QVector<vtkIdType> badIdsUnion;

    // For combo mode: per-rule breakdown.
    QVector<QVector<vtkIdType>> badIdsByRule;
    QVector<int> supportedByRule;
};

QVector<RuleCondition> collectRuleConditionsFromTable(QTableWidget* table);
QString formatComboRuleText(const QVector<RuleCondition>& conds, const QString& logicOp); // logicOp: "and"/"or"

// Pure evaluation (no rendering, no UI access).
BadCellEvalResult evaluateBadCellsSingle(vtkDataSet* mesh,
                                        vtkDataArray* qualityArray,
                                        const QString& metricKey,
                                        bool isGE,
                                        double threshold);

BadCellEvalResult evaluateBadCellsCombo(vtkDataSet* mesh,
                                       const QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCache,
                                       const QVector<RuleCondition>& rules,
                                       const QString& logicOp); // "and"/"or"
} // namespace vtklasttry

