#include "UiStateController.h"

#include <QPushButton>
#include <QStatusBar>
#include <QTextEdit>

#include "UiLog.h"

namespace vtklasttry
{
void UiStateController::setComputing(QPushButton* computeBtn, QPushButton* importBtn, bool computing)
{
    if (computeBtn)
        computeBtn->setEnabled(!computing);
    if (importBtn)
        importBtn->setEnabled(!computing);
}

void UiStateController::showStatusComputing(QStatusBar* sb, QTextEdit* logBox)
{
    if (sb)
        sb->showMessage(QString::fromWCharArray(L"\u6B63\u5728\u8BA1\u7B97\u2026"));
    if (logBox)
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u5F00\u59CB\u8BA1\u7B97\u8D28\u91CF\u6307\u6807\u2026"));
}

void UiStateController::showStatusComputeCanceled(QStatusBar* sb, QTextEdit* logBox)
{
    if (sb)
        sb->showMessage(QString::fromWCharArray(L"\u5DF2\u53D6\u6D88\u8BA1\u7B97\u3002"));
    if (logBox)
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u7528\u6237\u53D6\u6D88\u4E86\u8BA1\u7B97\u3002"));
}

void UiStateController::showStatusComputeFailed(QStatusBar* sb, QTextEdit* logBox)
{
    if (sb)
        sb->showMessage(QString::fromWCharArray(L"\u8D28\u91CF\u8BA1\u7B97\u5931\u8D25\u3002"));
    if (logBox)
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u8D28\u91CF\u8BA1\u7B97\u5931\u8D25\u3002"));
}

void UiStateController::showStatusComputeDone(QStatusBar* sb)
{
    if (sb)
        sb->showMessage(QString::fromWCharArray(L"\u8BA1\u7B97\u5B8C\u6210\u3002"));
}
} // namespace vtklasttry

