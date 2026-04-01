#pragma once

#include <QHash>
#include <QString>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QTextEdit;
class QTableWidget;

class vtkDataArray;
class vtkDataSet;
template <typename T> class vtkSmartPointer;

namespace vtklasttry
{
class TemplateController
{
public:
    static void applyThresholdTemplate(QCheckBox* enableComboRuleCheck,
                                       QComboBox* templateCombo,
                                       QComboBox* metricCombo,
                                       QComboBox* badRuleCombo,
                                       QDoubleSpinBox* thresholdSpin,
                                       QTextEdit* logBox,
                                       vtkSmartPointer<vtkDataSet> mesh,
                                       vtkSmartPointer<vtkDataArray> activeQualityArray,
                                       const QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCache);
};
} // namespace vtklasttry

