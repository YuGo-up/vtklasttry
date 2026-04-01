#include "RuleTablePanel.h"

#include "QualitySupport.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QMetaObject>
#include <QStatusBar>
#include <QTableWidget>
#include <QWidget>

#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkSmartPointer.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace vtklasttry
{
static void setEnabledSafe(QWidget* w, bool en)
{
    if (w)
        w->setEnabled(en);
}

double RuleTablePanel::adaptiveQuantileForMetricKey(vtkDataSet* mesh, vtkDataArray* qualityArray, const QString& metricKey, double q)
{
    if (!mesh || !qualityArray)
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
        const double v = qualityArray->GetTuple1(i);
        if (!vtklasttry::isFinite(v))
            continue;
        vals.push_back(v);
    }
    if (vals.empty())
        return std::numeric_limits<double>::quiet_NaN();

    double qq = q;
    if (qq < 0.0) qq = 0.0;
    if (qq > 1.0) qq = 1.0;
    const size_t k = static_cast<size_t>(qq * (vals.size() - 1));
    std::nth_element(vals.begin(), vals.begin() + static_cast<std::ptrdiff_t>(k), vals.end());
    return vals[k];
}

void RuleTablePanel::onComboRuleToggled(bool checked,
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
                                        const char* slotAnyRuleTemplateChanged)
{
    if (!primaryMetricCombo || !primaryBadRuleCombo || !primaryThresholdSpin || !logicOpCombo || !ruleTable || !templateCombo)
        return;

    primaryMetricCombo->setEnabled(!checked);
    primaryBadRuleCombo->setEnabled(!checked);
    primaryThresholdSpin->setEnabled(!checked);
    setEnabledSafe(labelMetric, !checked);
    setEnabledSafe(labelThreshold, !checked);
    setEnabledSafe(labelBadRule, !checked);

    logicOpCombo->setEnabled(checked);
    setEnabledSafe(labelLogicOp, checked);
    setEnabledSafe(labelRuleTable, checked);
    ruleTable->setEnabled(checked);
    setEnabledSafe(addRowBtn, checked);
    setEnabledSafe(removeRowBtn, checked);
    setEnabledSafe(labelTemplate, !checked);
    templateCombo->setEnabled(!checked);

    if (checked)
    {
        if (ruleTable->rowCount() < 2)
        {
            ruleTable->setRowCount(2);
            RuleTablePanel::setupRuleTableRowWidgets(ruleTable, 0, primaryMetricCombo, primaryBadRuleCombo, primaryThresholdSpin, templateCombo,
                                                     mainReceiverForSlots, slotMetricOrThresholdChanged, slotAnyRuleTemplateChanged);
            RuleTablePanel::setupRuleTableRowWidgets(ruleTable, 1, primaryMetricCombo, primaryBadRuleCombo, primaryThresholdSpin, templateCombo,
                                                     mainReceiverForSlots, slotMetricOrThresholdChanged, slotAnyRuleTemplateChanged);
        }
        if (QComboBox* mcb = qobject_cast<QComboBox*>(ruleTable->cellWidget(0, 0)))
            mcb->setCurrentIndex(primaryMetricCombo->currentIndex());
        if (QComboBox* rcb = qobject_cast<QComboBox*>(ruleTable->cellWidget(0, 1)))
            rcb->setCurrentIndex(primaryBadRuleCombo->currentIndex());
        if (QDoubleSpinBox* sp = qobject_cast<QDoubleSpinBox*>(ruleTable->cellWidget(0, 2)))
            sp->setValue(primaryThresholdSpin->value());
        RuleTablePanel::refreshRuleTableRowProperty(ruleTable);
    }

    if (statusBar)
    {
        statusBar->showMessage(QString::fromWCharArray(
            L"\u591A\u6307\u6807\u7EC4\u5408\u89C4\u5219\u5DF2%1\uFF0C\u8BF7\u70B9\u51FB\u201C\u8BA1\u7B97\u5E76\u663E\u793A\u201D\u66F4\u65B0\u7ED3\u679C\u3002")
                                   .arg(checked ? QString::fromWCharArray(L"\u542F\u7528") : QString::fromWCharArray(L"\u5173\u95ED")));
    }
}

void RuleTablePanel::fillMetricCombo(QComboBox* primaryMetricCombo, QComboBox* cb, const QString& selectKey)
{
    if (!primaryMetricCombo || !cb)
        return;
    cb->clear();
    for (int i = 0; i < primaryMetricCombo->count(); ++i)
        cb->addItem(primaryMetricCombo->itemText(i), primaryMetricCombo->itemData(i));
    if (!selectKey.isEmpty())
    {
        const int idx = cb->findData(selectKey);
        if (idx >= 0) cb->setCurrentIndex(idx);
    }
}

void RuleTablePanel::fillTemplateCombo(QComboBox* primaryTemplateCombo, QComboBox* cb, const QString& selectKey)
{
    if (!cb)
        return;
    cb->clear();
    if (primaryTemplateCombo && primaryTemplateCombo->count() > 0)
    {
        for (int i = 0; i < primaryTemplateCombo->count(); ++i)
            cb->addItem(primaryTemplateCombo->itemText(i), primaryTemplateCombo->itemData(i));
    }
    else
    {
        cb->addItem(QString::fromWCharArray(L"\u81EA\u5B9A\u4E49"), "custom");
        cb->addItem(QString::fromWCharArray(L"\u5E38\u7528\u6807\u51C6\uFF08\u4E00\u822C\uFF09"), "std_normal");
        cb->addItem(QString::fromWCharArray(L"\u5E38\u7528\u6807\u51C6\uFF08\u4E25\u683C\uFF09"), "std_strict");
        cb->addItem(QString::fromWCharArray(L"\u5E38\u7528\u6807\u51C6\uFF08\u5BBD\u677E\uFF09"), "std_loose");
        cb->addItem(QString::fromWCharArray(L"\u81EA\u9002\u5E94\uFF08\u5206\u4F4D\u6570\uFF09"), "adaptive");
    }
    if (!selectKey.isEmpty())
    {
        const int idx = cb->findData(selectKey);
        if (idx >= 0) cb->setCurrentIndex(idx);
    }
    else
        cb->setCurrentIndex(0);
}

void RuleTablePanel::initRuleTableDefaults(QTableWidget* ruleTable,
                                           QComboBox* primaryMetricCombo,
                                           QComboBox* primaryBadRuleCombo,
                                           QDoubleSpinBox* primaryThresholdSpin,
                                           QComboBox* templateCombo,
                                           QObject* mainReceiverForSlots,
                                           const char* slotMetricOrThresholdChanged,
                                           const char* slotAnyRuleTemplateChanged)
{
    if (!ruleTable)
        return;
    ruleTable->setRowCount(2);
    RuleTablePanel::setupRuleTableRowWidgets(ruleTable, 0, primaryMetricCombo, primaryBadRuleCombo, primaryThresholdSpin, templateCombo,
                                             mainReceiverForSlots, slotMetricOrThresholdChanged, slotAnyRuleTemplateChanged);
    RuleTablePanel::setupRuleTableRowWidgets(ruleTable, 1, primaryMetricCombo, primaryBadRuleCombo, primaryThresholdSpin, templateCombo,
                                             mainReceiverForSlots, slotMetricOrThresholdChanged, slotAnyRuleTemplateChanged);
    RuleTablePanel::refreshRuleTableRowProperty(ruleTable);
}

void RuleTablePanel::setupRuleTableRowWidgets(QTableWidget* ruleTable,
                                              int row,
                                              QComboBox* primaryMetricCombo,
                                              QComboBox* primaryBadRuleCombo,
                                              QDoubleSpinBox* primaryThresholdSpin,
                                              QComboBox* templateCombo,
                                              QObject* mainReceiverForSlots,
                                              const char* slotMetricOrThresholdChanged,
                                              const char* slotAnyRuleTemplateChanged)
{
    if (!ruleTable)
        return;

    QComboBox* mcb = new QComboBox(ruleTable);
    RuleTablePanel::fillMetricCombo(primaryMetricCombo, mcb);
    if (row == 0 && primaryMetricCombo)
        mcb->setCurrentIndex(primaryMetricCombo->currentIndex());
    else if (row == 1 && primaryMetricCombo && primaryMetricCombo->count() > 1)
        mcb->setCurrentIndex(1);
    ruleTable->setCellWidget(row, 0, mcb);
    if (mainReceiverForSlots && slotMetricOrThresholdChanged)
    {
        QObject::connect(mcb, QOverload<int>::of(&QComboBox::currentIndexChanged), mainReceiverForSlots,
                         [mainReceiverForSlots, slotMetricOrThresholdChanged]() { QMetaObject::invokeMethod(mainReceiverForSlots, slotMetricOrThresholdChanged); });
    }

    QComboBox* rcb = new QComboBox(ruleTable);
    rcb->addItem(QString::fromWCharArray(L"\u574F\uFF1Aq \u2265 \u9608\u503C"), "ge");
    rcb->addItem(QString::fromWCharArray(L"\u574F\uFF1Aq \u2264 \u9608\u503C"), "le");
    if (row == 0 && primaryBadRuleCombo)
        rcb->setCurrentIndex(primaryBadRuleCombo->currentIndex());
    ruleTable->setCellWidget(row, 1, rcb);
    if (mainReceiverForSlots && slotMetricOrThresholdChanged)
    {
        QObject::connect(rcb, QOverload<int>::of(&QComboBox::currentIndexChanged), mainReceiverForSlots,
                         [mainReceiverForSlots, slotMetricOrThresholdChanged]() { QMetaObject::invokeMethod(mainReceiverForSlots, slotMetricOrThresholdChanged); });
    }

    QDoubleSpinBox* sp = new QDoubleSpinBox(ruleTable);
    sp->setDecimals(6);
    sp->setRange(0.0, 1e9);
    if (row == 0 && primaryThresholdSpin)
        sp->setValue(primaryThresholdSpin->value());
    else
        sp->setValue(5.0);
    ruleTable->setCellWidget(row, 2, sp);
    if (mainReceiverForSlots && slotMetricOrThresholdChanged)
    {
        QObject::connect(sp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), mainReceiverForSlots,
                         [mainReceiverForSlots, slotMetricOrThresholdChanged]() { QMetaObject::invokeMethod(mainReceiverForSlots, slotMetricOrThresholdChanged); });
    }

    QComboBox* tcb = new QComboBox(ruleTable);
    RuleTablePanel::fillTemplateCombo(templateCombo, tcb, QStringLiteral("custom"));
    tcb->setProperty("ruleRow", row);
    ruleTable->setCellWidget(row, 3, tcb);
    if (mainReceiverForSlots && slotAnyRuleTemplateChanged)
    {
        QObject::connect(tcb, QOverload<int>::of(&QComboBox::currentIndexChanged), mainReceiverForSlots,
                         [mainReceiverForSlots, slotAnyRuleTemplateChanged]() { QMetaObject::invokeMethod(mainReceiverForSlots, slotAnyRuleTemplateChanged); });
    }
}

void RuleTablePanel::refreshRuleTableRowProperty(QTableWidget* ruleTable)
{
    if (!ruleTable)
        return;
    const int n = ruleTable->rowCount();
    for (int r = 0; r < n; ++r)
    {
        if (QComboBox* tcb = qobject_cast<QComboBox*>(ruleTable->cellWidget(r, 3)))
            tcb->setProperty("ruleRow", r);
    }
}

void RuleTablePanel::onAddRuleRow(QTableWidget* ruleTable,
                                  QComboBox* primaryMetricCombo,
                                  QComboBox* primaryBadRuleCombo,
                                  QDoubleSpinBox* primaryThresholdSpin,
                                  QComboBox* templateCombo,
                                  QStatusBar* statusBar,
                                  bool comboEnabled,
                                  QObject* mainReceiverForSlots,
                                  const char* slotMetricOrThresholdChanged,
                                  const char* slotAnyRuleTemplateChanged)
{
    if (!ruleTable || !comboEnabled)
        return;
    const int r = ruleTable->rowCount();
    ruleTable->insertRow(r);
    RuleTablePanel::setupRuleTableRowWidgets(ruleTable, r, primaryMetricCombo, primaryBadRuleCombo, primaryThresholdSpin, templateCombo,
                                             mainReceiverForSlots, slotMetricOrThresholdChanged, slotAnyRuleTemplateChanged);
    RuleTablePanel::refreshRuleTableRowProperty(ruleTable);
    ruleTable->selectRow(r);
    if (statusBar)
        statusBar->showMessage(QString::fromWCharArray(L"\u5DF2\u6DFB\u52A0\u7EC4\u5408\u6761\u4EF6\u884C\uFF0C\u8BF7\u70B9\u51FB\u201C\u8BA1\u7B97\u5E76\u663E\u793A\u201D\u3002"));
}

void RuleTablePanel::onRemoveRuleRow(QTableWidget* ruleTable, QStatusBar* statusBar, bool comboEnabled, QWidget* parentForDialogs)
{
    if (!ruleTable || !comboEnabled)
        return;
    const int n = ruleTable->rowCount();
    if (n <= 2)
    {
        QMessageBox::information(parentForDialogs,
                                 QString::fromWCharArray(L"\u63D0\u793A"),
                                 QString::fromWCharArray(L"\u81F3\u5C11\u4FDD\u7559 2 \u6761\u7EC4\u5408\u6761\u4EF6\u3002"));
        return;
    }
    const int r = ruleTable->currentRow();
    if (r < 0 || r >= n)
        return;
    ruleTable->removeRow(r);
    RuleTablePanel::refreshRuleTableRowProperty(ruleTable);
    if (statusBar)
        statusBar->showMessage(QString::fromWCharArray(L"\u5DF2\u5220\u9664\u7EC4\u5408\u6761\u4EF6\u884C\uFF0C\u8BF7\u70B9\u51FB\u201C\u8BA1\u7B97\u5E76\u663E\u793A\u201D\u3002"));
}

void RuleTablePanel::onAnyRuleTemplateChanged(QObject* senderObj,
                                              QTableWidget* ruleTable,
                                              QStatusBar* statusBar,
                                              vtkSmartPointer<vtkDataSet> mesh,
                                              const QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCache)
{
    auto* tcb = qobject_cast<QComboBox*>(senderObj);
    if (!tcb)
        return;
    const int row = tcb->property("ruleRow").toInt();
    RuleTablePanel::applyTemplateToRuleRow(ruleTable, row, mesh, qualityCache);
    if (statusBar)
    {
        statusBar->showMessage(QString::fromWCharArray(
            L"\u7B2C %1 \u884C\u6A21\u677F\u5DF2\u5E94\u7528\uFF0C\u8BF7\u70B9\u51FB\u201C\u8BA1\u7B97\u5E76\u663E\u793A\u201D\u3002").arg(row + 1));
    }
}

void RuleTablePanel::applyTemplateToRuleRow(QTableWidget* ruleTable,
                                            int row,
                                            vtkSmartPointer<vtkDataSet> mesh,
                                            const QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCache)
{
    if (!ruleTable || row < 0 || row >= ruleTable->rowCount())
        return;
    auto* tcb = qobject_cast<QComboBox*>(ruleTable->cellWidget(row, 3));
    auto* mcb = qobject_cast<QComboBox*>(ruleTable->cellWidget(row, 0));
    auto* rcb = qobject_cast<QComboBox*>(ruleTable->cellWidget(row, 1));
    auto* sp = qobject_cast<QDoubleSpinBox*>(ruleTable->cellWidget(row, 2));
    if (!tcb || !mcb || !rcb || !sp)
        return;
    const QString tpl = tcb->currentData().toString();
    if (tpl == "custom")
        return;
    const QString metricKey = mcb->currentData().toString();

    vtkDataArray* arr = (qualityCache.contains(metricKey) && qualityCache.value(metricKey)) ? qualityCache.value(metricKey) : nullptr;
    auto aq = [&](double qq) { return adaptiveQuantileForMetricKey(mesh.GetPointer(), arr, metricKey, qq); };

    double thr = sp->value();
    QString rule = "ge";

    if (metricKey == "aspect")
    {
        rule = "ge";
        thr = 5.0;
        if (tpl == "std_strict") thr = 3.0;
        else if (tpl == "std_normal") thr = 5.0;
        else if (tpl == "std_loose") thr = 10.0;
        else if (tpl == "adaptive") thr = aq(0.95);
    }
    else if (metricKey == "skew")
    {
        rule = "ge";
        thr = 0.5;
        if (tpl == "std_strict") thr = 0.3;
        else if (tpl == "std_normal") thr = 0.5;
        else if (tpl == "std_loose") thr = 0.8;
        else if (tpl == "adaptive") thr = aq(0.95);
    }
    else if (metricKey == "scaled_jacobian" || metricKey == "jacobian")
    {
        rule = "le";
        thr = (metricKey == "jacobian") ? 0.1 : 0.2;
        if (tpl == "std_strict") thr = (metricKey == "jacobian") ? 0.2 : 0.3;
        else if (tpl == "std_normal") thr = (metricKey == "jacobian") ? 0.1 : 0.2;
        else if (tpl == "std_loose") thr = (metricKey == "jacobian") ? 0.05 : 0.1;
        else if (tpl == "adaptive") thr = aq(0.05);
    }
    else if (metricKey == "jacobian_ratio")
    {
        rule = "ge";
        thr = 10.0;
        if (tpl == "std_strict") thr = 5.0;
        else if (tpl == "std_normal") thr = 10.0;
        else if (tpl == "std_loose") thr = 30.0;
        else if (tpl == "adaptive") thr = aq(0.95);
    }
    else if (metricKey == "min_angle")
    {
        rule = "le";
        thr = 25.0;
        if (tpl == "std_strict") thr = 30.0;
        else if (tpl == "std_normal") thr = 25.0;
        else if (tpl == "std_loose") thr = 15.0;
        else if (tpl == "adaptive") thr = aq(0.05);
    }
    else if (metricKey == "max_angle")
    {
        rule = "ge";
        thr = 160.0;
        if (tpl == "std_strict") thr = 150.0;
        else if (tpl == "std_normal") thr = 160.0;
        else if (tpl == "std_loose") thr = 170.0;
        else if (tpl == "adaptive") thr = aq(0.95);
    }
    else if (metricKey == "min_edge_length")
    {
        rule = "le";
        thr = 1e-6;
        if (tpl == "std_strict") thr = 1e-5;
        else if (tpl == "std_normal") thr = 1e-6;
        else if (tpl == "std_loose") thr = 1e-8;
        else if (tpl == "adaptive") thr = aq(0.05);
    }
    else if (metricKey == "non_ortho")
    {
        rule = "ge";
        thr = 70.0;
        if (tpl == "std_strict") thr = 60.0;
        else if (tpl == "std_normal") thr = 70.0;
        else if (tpl == "std_loose") thr = 80.0;
        else if (tpl == "adaptive") thr = aq(0.95);
    }

    const int ruleIdx = rcb->findData(rule);
    if (ruleIdx >= 0)
        rcb->setCurrentIndex(ruleIdx);
    if (std::isfinite(thr))
        sp->setValue(thr);
}
} // namespace vtklasttry

