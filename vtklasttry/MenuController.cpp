#include "MenuController.h"

#include "ExportEngine.h"
#include "RuleEngine.h"
#include "UiLog.h"

#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QWidgetAction>

namespace vtklasttry
{
void MenuController::setupMenus(QMainWindow* w,
                                QMenuBar* menuBar,
                                QDockWidget* dockBadCells,
                                QDockWidget* dockLog,
                                const QObject* fileOpenReceiver,
                                const char* fileOpenSlot,
                                const QObject* exportCsvReceiver,
                                const char* exportCsvSlot,
                                const QObject* exportPngReceiver,
                                const char* exportPngSlot,
                                const QObject* exportMdReceiver,
                                const char* exportMdSlot)
{
    if (!w || !menuBar)
        return;

    // Menus: Import + View (reopen docks after closing).
    // Add visible black vertical separators between top-level menus.
    // Note: On some Windows styles, QMenuBar::separator is not painted; use QWidgetAction+QFrame instead.
    auto addMenuVSeparator = [&]() {
        auto* frame = new QFrame(menuBar);
        frame->setFrameShape(QFrame::VLine);
        frame->setFrameShadow(QFrame::Plain);
        frame->setLineWidth(1);
        frame->setStyleSheet(QStringLiteral("QFrame{color:#000000; background:#000000; margin:4px 6px;}"));
        auto* wa = new QWidgetAction(menuBar);
        wa->setDefaultWidget(frame);
        menuBar->addAction(wa);
    };

    QMenu* importMenu = menuBar->addMenu(QString::fromWCharArray(L"\u5BFC\u5165\u6587\u4EF6"));
    QAction* actOpen = importMenu->addAction(QString::fromWCharArray(L"\u6253\u5F00\u2026"));
    actOpen->setShortcut(QKeySequence::Open);
    if (fileOpenReceiver && fileOpenSlot)
        QObject::connect(actOpen, SIGNAL(triggered()), fileOpenReceiver, fileOpenSlot);

    addMenuVSeparator();

    QMenu* exportMenu = menuBar->addMenu(QString::fromWCharArray(L"\u5BFC\u51FA"));
    QAction* actExportCsv = exportMenu->addAction(QString::fromWCharArray(L"\u5BFC\u51FA\u574F\u5355\u5143CSV\u2026"));
    if (exportCsvReceiver && exportCsvSlot)
        QObject::connect(actExportCsv, SIGNAL(triggered()), exportCsvReceiver, exportCsvSlot);
    QAction* actExportPng = exportMenu->addAction(QString::fromWCharArray(L"\u5BFC\u51FA\u622A\u56FE\uFF08PNG\uFF09\u2026"));
    if (exportPngReceiver && exportPngSlot)
        QObject::connect(actExportPng, SIGNAL(triggered()), exportPngReceiver, exportPngSlot);
    QAction* actExportMd = exportMenu->addAction(QString::fromWCharArray(L"\u5BFC\u51FA\u62A5\u544A\uFF08MD\uFF09\u2026"));
    if (exportMdReceiver && exportMdSlot)
        QObject::connect(actExportMd, SIGNAL(triggered()), exportMdReceiver, exportMdSlot);

    addMenuVSeparator();

    QMenu* viewMenu = menuBar->addMenu(QString::fromWCharArray(L"\u89C6\u56FE"));
    if (dockBadCells)
        viewMenu->addAction(dockBadCells->toggleViewAction());
    if (dockLog)
        viewMenu->addAction(dockLog->toggleViewAction());
}

void MenuController::onExportBadCellsCsv(QMainWindow* parent,
                                         QTextEdit* logBox,
                                         QComboBox* metricCombo,
                                         QComboBox* badRuleCombo,
                                         QDoubleSpinBox* thresholdSpin,
                                         QCheckBox* enableComboRuleCheck,
                                         QComboBox* comboLogicCombo,
                                         QTableWidget* ruleTable,
                                         const QString& currentFilePath,
                                         const QString& comboRuleText,
                                         vtkSmartPointer<vtkDataSet> mesh,
                                         vtkSmartPointer<vtkDataArray> activeQualityArray,
                                         const QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCache,
                                         const QVector<vtkIdType>& lastBadCellIds,
                                         QString* inOutLastExportCsvPath)
{
    if (!parent || !mesh || !activeQualityArray || !metricCombo || !badRuleCombo || !thresholdSpin || !enableComboRuleCheck || !comboLogicCombo || !ruleTable)
        return;

    if (!mesh || !activeQualityArray)
    {
        QMessageBox::information(parent,
                                 QString::fromWCharArray(L"\u63D0\u793A"),
                                 QString::fromWCharArray(L"\u8BF7\u5148\u52A0\u8F7D\u7F51\u683C\u5E76\u8BA1\u7B97\u8D28\u91CF\u540E\u518D\u5BFC\u51FA\u3002"));
        return;
    }

    const QString defaultName = QString("bad_cells_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString path = QFileDialog::getSaveFileName(parent,
                                                      QString::fromWCharArray(L"\u5BFC\u51FA\u574F\u5355\u5143 CSV"),
                                                      defaultName,
                                                      "CSV (*.csv)");
    if (path.isEmpty())
        return;

    const QString metricKey = metricCombo->currentData().toString();
    const QString metricName = metricCombo->currentText();
    const QString ruleKey = badRuleCombo->currentData().toString();
    const QString ruleText = badRuleCombo->currentText();
    const double thr = thresholdSpin->value();
    const bool comboEnabled = enableComboRuleCheck->isChecked();
    const QVector<vtklasttry::RuleCondition> tableConds = vtklasttry::collectRuleConditionsFromTable(ruleTable);

    vtklasttry::ExportCsvInput in;
    in.sourcePath = currentFilePath;
    in.displayMetricKey = metricKey;
    in.displayMetricName = metricName;
    in.comboEnabled = comboEnabled;
    in.ruleKey = ruleKey;
    in.ruleText = ruleText;
    in.threshold = thr;
    in.comboLogic = comboLogicCombo->currentData().toString();
    in.comboRuleText = comboRuleText;
    for (int r = 0; r < tableConds.size(); ++r)
    {
        vtklasttry::RuleCondition rc;
        rc.metricKey = tableConds[r].metricKey;
        rc.isGE = tableConds[r].isGE;
        rc.threshold = tableConds[r].threshold;
        if (QComboBox* tcb = qobject_cast<QComboBox*>(ruleTable->cellWidget(r, 3)))
            rc.templateKey = tcb->currentData().toString();
        in.comboRules.push_back(rc);
    }
    in.mesh = mesh;
    in.displayQualityArray = activeQualityArray;
    in.qualityCache = qualityCache;
    in.badCellIds = lastBadCellIds;

    QString err;
    if (!vtklasttry::exportBadCellsCsv(path, in, &err))
    {
        QMessageBox::warning(parent, QString::fromWCharArray(L"\u5BFC\u51FA\u5931\u8D25"), err.isEmpty() ? QString::fromWCharArray(L"\u5BFC\u51FA\u5931\u8D25\u3002") : err);
        return;
    }

    if (inOutLastExportCsvPath)
        *inOutLastExportCsvPath = path;
    if (logBox)
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u5DF2\u5BFC\u51FA CSV\uFF1A%1").arg(path));
}

void MenuController::onExportScreenshotPng(QMainWindow* parent,
                                          QTextEdit* logBox,
                                          vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow,
                                          QString* inOutLastExportPngPath)
{
    if (!parent || !renderWindow)
        return;

    const QString defaultName = QString("screenshot_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString path = QFileDialog::getSaveFileName(parent,
                                                      QString::fromWCharArray(L"\u5BFC\u51FA\u622A\u56FE PNG\uFF08\u542B\u6807\u5C3A\uFF09"),
                                                      defaultName,
                                                      "PNG (*.png)");
    if (path.isEmpty())
        return;

    QString err;
    if (!vtklasttry::exportScreenshotPng(renderWindow, path, &err))
    {
        QMessageBox::warning(parent, QString::fromWCharArray(L"\u5BFC\u51FA\u5931\u8D25"), err.isEmpty() ? QString::fromWCharArray(L"\u5BFC\u51FA\u5931\u8D25\u3002") : err);
        return;
    }

    if (inOutLastExportPngPath)
        *inOutLastExportPngPath = path;
    if (logBox)
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u5DF2\u5BFC\u51FA\u622A\u56FE\uFF1A%1").arg(path));
}

void MenuController::onExportReportMd(QMainWindow* parent,
                                      QTextEdit* logBox,
                                      QComboBox* metricCombo,
                                      QComboBox* badRuleCombo,
                                      QComboBox* templateCombo,
                                      QDoubleSpinBox* thresholdSpin,
                                      QCheckBox* enableComboRuleCheck,
                                      const QString& currentFilePath,
                                      const QString& importPipelineText,
                                      const QString& meshClassText,
                                      const QString& cellTypeStatsTableMd,
                                      const QString& metricSupportSummary,
                                      const QString& comboRuleTextMaybeEmpty,
                                      const QString& lastExportCsvPath,
                                      const QString& lastExportPngPath,
                                      vtkSmartPointer<vtkDataSet> mesh,
                                      vtkSmartPointer<vtkDataArray> qualityArray,
                                      const QVector<vtkIdType>& lastBadCellIds,
                                      int lastTotalCellsForStats,
                                      QString* outSavedReportPath)
{
    if (!parent)
        return;
    if (!mesh || !qualityArray)
    {
        QMessageBox::information(parent,
                                 QString::fromWCharArray(L"\u63D0\u793A"),
                                 QString::fromWCharArray(L"\u8BF7\u5148\u52A0\u8F7D\u7F51\u683C\u5E76\u8BA1\u7B97\u8D28\u91CF\u540E\u518D\u5BFC\u51FA\u62A5\u544A\u3002"));
        return;
    }

    const QString defaultName = QString("report_%1.md").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString path = QFileDialog::getSaveFileName(parent,
                                                      QString::fromWCharArray(L"\u5BFC\u51FA Markdown \u62A5\u544A"),
                                                      defaultName,
                                                      "Markdown (*.md)");
    if (path.isEmpty())
        return;

    const QString metricKey = metricCombo ? metricCombo->currentData().toString() : QString();
    const QString metricName = metricCombo ? metricCombo->currentText() : QString();
    const QString ruleKey = badRuleCombo ? badRuleCombo->currentData().toString() : QString();
    const QString ruleText = badRuleCombo ? badRuleCombo->currentText() : QString();
    const QString tplKey = templateCombo ? templateCombo->currentData().toString() : QString();
    const QString tplText = templateCombo ? templateCombo->currentText() : QString();
    const double thr = thresholdSpin ? thresholdSpin->value() : 0.0;

    double range[2] = { 0.0, 0.0 };
    qualityArray->GetRange(range);

    const int total = (lastTotalCellsForStats > 0) ? lastTotalCellsForStats : static_cast<int>(mesh->GetNumberOfCells());
    const int bad = static_cast<int>(lastBadCellIds.size());
    const double ratio = (total > 0) ? (100.0 * bad / total) : 0.0;

    QFile tplFile(":/mainwindow/report_template_zh.md");
    if (!tplFile.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(parent,
                             QString::fromWCharArray(L"\u5BFC\u51FA\u5931\u8D25"),
                             QString::fromWCharArray(L"\u65E0\u6CD5\u8BFB\u53D6\u62A5\u544A\u6A21\u677F\u8D44\u6E90\u3002"));
        return;
    }
    const QByteArray tplBytes = tplFile.readAll();
    const QString tpl = QString::fromUtf8(tplBytes);

    vtklasttry::ExportReportInput in;
    in.metricKey = metricKey;
    in.metricName = metricName;
    in.ruleKey = ruleKey;
    in.ruleText = ruleText;
    in.templateKey = tplKey;
    in.templateText = tplText;
    in.threshold = thr;
    in.sourcePath = currentFilePath;
    in.importPipelineText = importPipelineText;
    in.meshClassText = meshClassText;
    in.cellTypeStatsTableMd = cellTypeStatsTableMd;
    in.metricSupportSummary = metricSupportSummary;
    in.comboRuleText = (enableComboRuleCheck && enableComboRuleCheck->isChecked()) ? comboRuleTextMaybeEmpty : QString();
    in.csvPath = lastExportCsvPath;
    in.pngPath = lastExportPngPath;
    in.mesh = mesh;
    in.qualityArray = qualityArray;
    in.badCellIds = lastBadCellIds;
    in.totalCells = total;
    in.badCells = bad;
    in.badRatioPct = ratio;
    in.qualityRangeMin = range[0];
    in.qualityRangeMax = range[1];
    in.templateMarkdown = tpl;

    QString outMd;
    QString err;
    if (!vtklasttry::buildReportMarkdown(in, &outMd, &err))
    {
        QMessageBox::warning(parent,
                             QString::fromWCharArray(L"\u5BFC\u51FA\u5931\u8D25"),
                             err.isEmpty() ? QString::fromWCharArray(L"\u65E0\u6CD5\u751F\u6210\u62A5\u544A\u5185\u5BB9\u3002") : err);
        return;
    }

    if (!vtklasttry::writeUtf8TextFile(path, outMd, &err))
    {
        QMessageBox::warning(parent,
                             QString::fromWCharArray(L"\u5BFC\u51FA\u5931\u8D25"),
                             err.isEmpty() ? QString::fromWCharArray(L"\u6587\u4EF6\u4FDD\u5B58\u5931\u8D25\u3002") : err);
        return;
    }

    if (outSavedReportPath)
        *outSavedReportPath = path;
    if (logBox)
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u5DF2\u5BFC\u51FA\u62A5\u544A\uFF1A%1").arg(path));
}
} // namespace vtklasttry

