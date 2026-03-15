#pragma once

#include <QDialog>

class QDoubleSpinBox;
class QDialogButtonBox;

class ThresholdDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ThresholdDialog(double aspectThreshold, double skewThreshold, QWidget* parent = nullptr);

    double aspectThreshold() const;
    double skewThreshold() const;

private:
    QDoubleSpinBox* m_aspectSpin = nullptr;
    QDoubleSpinBox* m_skewSpin = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
};
