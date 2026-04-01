#pragma once

#include <QString>
#include <QHash>

class QComboBox;
class QDoubleSpinBox;
class QStatusBar;
class QTableWidget;
class QObject;
class QWidget;

class vtkDataArray;
class vtkDataSet;
template <typename T> class vtkSmartPointer;

namespace vtklasttry
{
class RuleTablePanel
{
public:
    static void onComboRuleToggled(bool checked,
                                   QComboBox* primaryMetricCombo,
                                   QComboBox* primaryBadRuleCombo,
                                   QDoubleSpinBox* primaryThresholdSpin,
                                   QWidget* labelMetric,
                                   QWidget* labelThreshold,
                                   QWidget* labelBadRule,
                                   QComboBox* logicOpCombo,
                                   QWidget* labelLogicOp,
                                   QWidget* labelRuleTable,
                                   QTableWidget* ruleTable,
                                   QWidget* addRowBtn,
                                   QWidget* removeRowBtn,
                                   QWidget* labelTemplate,
                                   QComboBox* templateCombo,
                                   QStatusBar* statusBar,
                                   QObject* mainReceiverForSlots,
                                   const char* slotMetricOrThresholdChanged,
                                   const char* slotAnyRuleTemplateChanged);

    static void initRuleTableDefaults(QTableWidget* ruleTable,
                                      QComboBox* primaryMetricCombo,
                                      QComboBox* primaryBadRuleCombo,
                                      QDoubleSpinBox* primaryThresholdSpin,
                                      QComboBox* templateCombo,
                                      QObject* mainReceiverForSlots,
                                      const char* slotMetricOrThresholdChanged,
                                      const char* slotAnyRuleTemplateChanged);
    static void setupRuleTableRowWidgets(QTableWidget* ruleTable,
                                         int row,
                                         QComboBox* primaryMetricCombo,
                                         QComboBox* primaryBadRuleCombo,
                                         QDoubleSpinBox* primaryThresholdSpin,
                                         QComboBox* templateCombo,
                                         QObject* mainReceiverForSlots,
                                         const char* slotMetricOrThresholdChanged,
                                         const char* slotAnyRuleTemplateChanged);
    static void refreshRuleTableRowProperty(QTableWidget* ruleTable);

    static void onAddRuleRow(QTableWidget* ruleTable,
                             QComboBox* primaryMetricCombo,
                             QComboBox* primaryBadRuleCombo,
                             QDoubleSpinBox* primaryThresholdSpin,
                             QComboBox* templateCombo,
                             QStatusBar* statusBar,
                             bool comboEnabled,
                             QObject* mainReceiverForSlots,
                             const char* slotMetricOrThresholdChanged,
                             const char* slotAnyRuleTemplateChanged);
    static void onRemoveRuleRow(QTableWidget* ruleTable, QStatusBar* statusBar, bool comboEnabled, QWidget* parentForDialogs);
    static void onAnyRuleTemplateChanged(QObject* senderObj,
                                         QTableWidget* ruleTable,
                                         QStatusBar* statusBar,
                                         vtkSmartPointer<vtkDataSet> mesh,
                                         const QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCache);
    static void applyTemplateToRuleRow(QTableWidget* ruleTable,
                                       int row,
                                       vtkSmartPointer<vtkDataSet> mesh,
                                       const QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCache);

    static void fillMetricCombo(QComboBox* primaryMetricCombo, QComboBox* cb, const QString& selectKey = QString());
    static void fillTemplateCombo(QComboBox* primaryTemplateCombo, QComboBox* cb, const QString& selectKey = QString());

private:
    static double adaptiveQuantileForMetricKey(vtkDataSet* mesh,
                                               vtkDataArray* qualityArray,
                                               const QString& metricKey,
                                               double q);
};
} // namespace vtklasttry

