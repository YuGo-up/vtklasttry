#pragma once

class QPushButton;
class QStatusBar;
class QTextEdit;

namespace vtklasttry
{
class UiStateController
{
public:
    static void setComputing(QPushButton* computeBtn, QPushButton* importBtn, bool computing);
    static void showStatusComputing(QStatusBar* sb, QTextEdit* logBox);
    static void showStatusComputeCanceled(QStatusBar* sb, QTextEdit* logBox);
    static void showStatusComputeFailed(QStatusBar* sb, QTextEdit* logBox);
    static void showStatusComputeDone(QStatusBar* sb);
};
} // namespace vtklasttry

