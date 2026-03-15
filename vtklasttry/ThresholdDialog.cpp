#include "ThresholdDialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

ThresholdDialog::ThresholdDialog(double aspectThreshold, double skewThreshold, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("质量阈值设置"));
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_aspectSpin = new QDoubleSpinBox(this);
    m_aspectSpin->setRange(1.0, 1000.0);
    m_aspectSpin->setDecimals(2);
    m_aspectSpin->setSingleStep(0.5);
    m_aspectSpin->setValue(aspectThreshold);

    m_skewSpin = new QDoubleSpinBox(this);
    m_skewSpin->setRange(0.0, 1.0);
    m_skewSpin->setDecimals(2);
    m_skewSpin->setSingleStep(0.05);
    m_skewSpin->setValue(skewThreshold);

    QFormLayout* form = new QFormLayout();
    form->addRow(tr("长宽比阈值(上限):"), m_aspectSpin);
    form->addRow(tr("扭曲度阈值(上限):"), m_skewSpin);
    layout->addLayout(form);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("确定"));
    m_buttons->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttons);
}

double ThresholdDialog::aspectThreshold() const
{
    return m_aspectSpin->value();
}

double ThresholdDialog::skewThreshold() const
{
    return m_skewSpin->value();
}
