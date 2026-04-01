#pragma once

class QAction;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QDockWidget;
class QMainWindow;
class QMenuBar;
class QTextEdit;
class QTableWidget;
class QObject;

class vtkDataArray;
class vtkDataSet;
class vtkGenericOpenGLRenderWindow;
template <typename T> class vtkSmartPointer;
using vtkIdType = long long;

#include <QHash>
#include <QString>
#include <QVector>

namespace vtklasttry
{
class MenuController
{
public:
    static void setupMenus(QMainWindow* w,
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
                           const char* exportMdSlot);

    static void onExportBadCellsCsv(QMainWindow* parent,
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
                                    QString* inOutLastExportCsvPath);

    static void onExportScreenshotPng(QMainWindow* parent,
                                     QTextEdit* logBox,
                                     vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow,
                                     QString* inOutLastExportPngPath);

    static void onExportReportMd(QMainWindow* parent,
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
                                QString* outSavedReportPath);
};
} // namespace vtklasttry

