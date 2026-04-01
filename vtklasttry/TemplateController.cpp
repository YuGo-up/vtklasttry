#include "TemplateController.h"

#include "ThresholdTemplates.h"
#include "UiLog.h"

#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkSmartPointer.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHash>
#include <QTextEdit>

namespace vtklasttry
{
void TemplateController::applyThresholdTemplate(QCheckBox* enableComboRuleCheck,
                                                QComboBox* templateCombo,
                                                QComboBox* metricCombo,
                                                QComboBox* badRuleCombo,
                                                QDoubleSpinBox* thresholdSpin,
                                                QTextEdit* logBox,
                                                vtkSmartPointer<vtkDataSet> mesh,
                                                vtkSmartPointer<vtkDataArray> activeQualityArray,
                                                const QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCache)
{
    if (!enableComboRuleCheck || !templateCombo || !metricCombo || !badRuleCombo || !thresholdSpin)
        return;

    if (enableComboRuleCheck->isChecked())
        return;
    const QString tpl = templateCombo->currentData().toString();
    if (tpl == "custom")
        return;

    const QString metricKey = metricCombo->currentData().toString();
    const QString metricName = metricCombo->currentText();

    vtkDataArray* qa = nullptr;
    if (qualityCache.contains(metricKey) && qualityCache.value(metricKey))
        qa = qualityCache.value(metricKey);
    else
        qa = activeQualityArray;
    vtklasttry::TemplateApplyInput in;
    in.templateKey = tpl;
    in.metricKey = metricKey;
    in.mesh = mesh.GetPointer();
    in.qualityArray = qa;
    const auto out = vtklasttry::applyThresholdTemplate(in);
    if (!out.ok)
        return;

    badRuleCombo->setCurrentIndex(badRuleCombo->findData(out.ruleKey));
    thresholdSpin->setValue(out.threshold);
    if (logBox)
    {
        vtklasttry::appendUiLog(logBox, QString("template=%1, metric=%2 -> rule=%3, thr=%4")
                                            .arg(tpl)
                                            .arg(metricName)
                                            .arg(out.ruleKey)
                                            .arg(out.threshold, 0, 'g', 8));
    }
}
} // namespace vtklasttry

