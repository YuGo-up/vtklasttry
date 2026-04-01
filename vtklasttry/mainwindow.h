#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_mainwindow.h"
#include "RuleEngine.h"
#include "QualityEngine.h"

#include <vtkSmartPointer.h>

#include <QListWidgetItem>
#include <QVector>
#include <QHash>
#include <QByteArray>
#include <array>
#include <atomic>

class QComboBox;
class QDoubleSpinBox;
template <typename T> class QFutureWatcher;
class QProgressDialog;

class vtkRenderer;
class vtkGenericOpenGLRenderWindow;
class vtkDataSet;
class vtkDataArray;
class vtkLookupTable;
class vtkScalarBarActor;
class vtkActor;
class vtkStringOutputWindow;
class vtkCellPicker;
class vtkCallbackCommand;
class vtkLegendBoxActor;
class vtkTextActor;

namespace vtklasttry { struct SceneViewRequest; struct OverlayRequest; struct ComputeRequest; struct AppState; struct QualityRequest; struct QualityState; struct MeshSelectRequest; struct MeshState; }
namespace vtklasttry { class SceneController; class BadCellsPanel; class RuleTablePanel; class MeshWorkflow; class ComputeWorkflow; class QualityWorkflow; }
namespace vtklasttry { class MenuController; }
namespace vtklasttry { class TemplateController; }

class mainwindow : public QMainWindow
{
    Q_OBJECT

public:
    mainwindow(QWidget *parent = nullptr);
    ~mainwindow();
    void pickFromInteractor();

    // --- SceneController accessors (replace friend access) ---
    // UI refs needed by scene/interaction layer
    QVTKOpenGLNativeWidget* vtkWidget() const { return ui.openGLWidget; }
    QTextEdit* uiTextLog() const { return ui.textLog; }
    QListWidget* uiBadCellsList() const { return ui.listBadCells; }
    QLabel* uiBadStatsLabel() const { return ui.labelBadStats; }
    bool uiIsolateSelectedBadChecked() const { return ui.checkIsolateSelectedBad->isChecked(); }
    QString uiCurrentMetricKey() const { return ui.comboMetric->currentData().toString(); }
    bool uiComboEnabledChecked() const { return ui.checkEnableComboRule->isChecked(); }
    bool uiShowBadChecked() const { return ui.checkShowBad->isChecked(); }

    // VTK/app state needed by scene/interaction layer
    vtkSmartPointer<vtkGenericOpenGLRenderWindow>& renderWindowSp() { return m_renderWindow; }
    vtkSmartPointer<vtkRenderer>& rendererSp() { return m_renderer; }
    vtkSmartPointer<vtkDataSet>& meshDataSp() { return m_meshData; }
    vtkSmartPointer<vtkDataArray>& qualityArraySp() { return m_qualityArray; }
    vtkSmartPointer<vtkLookupTable>& lutSp() { return m_lut; }
    vtkSmartPointer<vtkScalarBarActor>& scalarBarSp() { return m_scalarBar; }

    vtkSmartPointer<vtkActor>& meshActorSp() { return m_meshActor; }
    vtkSmartPointer<vtkActor>& badActorSp() { return m_badActor; }
    QVector<vtkSmartPointer<vtkActor>>& comboBadActorsRef() { return m_comboBadActors; }
    vtkSmartPointer<vtkLegendBoxActor>& comboLegendSp() { return m_comboLegend; }
    vtkSmartPointer<vtkTextActor>& comboLegendTitleSp() { return m_comboLegendTitle; }
    vtkSmartPointer<vtkActor>& selectedActorSp() { return m_selectedActor; }
    vtkSmartPointer<vtkActor>& isolatedActorSp() { return m_isolatedActor; }
    vtkIdType& selectedCellIdRef() { return m_selectedCellId; }
    vtkSmartPointer<vtkCellPicker>& cellPickerSp() { return m_cellPicker; }
    vtkSmartPointer<vtkCallbackCommand>& pickCallbackSp() { return m_pickCallback; }

    QHash<QString, vtkSmartPointer<vtkDataArray>>& qualityCacheRef() { return m_qualityCache; }
    QHash<vtkIdType, std::array<double, 3>>& comboBadColorByCellRef() { return m_comboBadColorByCell; }
    QVector<QByteArray>& comboLegendEntryUtf8Ref() { return m_comboLegendEntryUtf8; }
    QVector<vtkIdType>& lastBadCellIdsRef() { return m_lastBadCellIds; }
    int& lastTotalCellsForStatsRef() { return m_lastTotalCellsForStats; }
    void setComboLegendChromeVisiblePublic(bool visible) { setComboLegendChromeVisible(visible); }

private slots:
    void onSelectFile();
    void onComputeAndShow();
    void onMetricOrThresholdChanged();
    void onBadCellActivated(QListWidgetItem* item);
    void onExportBadCellsCsv();
    void onExportScreenshotPng();
    void onExportReportMd();
    void onIsolateSelectedBadToggled(bool checked);
    void onTemplateChanged(int idx);
    void onComboRuleToggled(bool checked);
    void onAddRuleRow();
    void onRemoveRuleRow();
    void onAnyRuleTemplateChanged();

private:
    Ui::mainwindowClass ui;

    QString m_currentFilePath;

    // VTK scene objects (kept alive for UI interactions)
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer> m_renderer;

    vtkSmartPointer<vtkDataSet> m_meshData;
    vtkSmartPointer<vtkDataArray> m_qualityArray;

    vtkSmartPointer<vtkLookupTable> m_lut;
    vtkSmartPointer<vtkScalarBarActor> m_scalarBar;

    vtkSmartPointer<vtkActor> m_meshActor;
    vtkSmartPointer<vtkActor> m_badActor;
    QVector<vtkSmartPointer<vtkActor>> m_comboBadActors;
    vtkSmartPointer<vtkLegendBoxActor> m_comboLegend;
    vtkSmartPointer<vtkTextActor> m_comboLegendTitle;
    vtkSmartPointer<vtkActor> m_selectedActor;
    vtkSmartPointer<vtkActor> m_isolatedActor;
    vtkIdType m_selectedCellId = -1;
    vtkSmartPointer<vtkCellPicker> m_cellPicker;
    vtkSmartPointer<vtkCallbackCommand> m_pickCallback;

    vtkSmartPointer<vtkStringOutputWindow> m_vtkLog;

    QString m_lastQualityMetricKey; // e.g. "aspect" / "skew"
    QHash<QString, vtkSmartPointer<vtkDataArray>> m_qualityCache; // metric_key -> array
    // In combo mode: cellId -> overlay color of the (last) matching rule.
    QHash<vtkIdType, std::array<double, 3>> m_comboBadColorByCell;
    // Keep UTF-8 strings alive for VTK legend entries (avoid dangling const char* from temporary QByteArray).
    QVector<QByteArray> m_comboLegendEntryUtf8;
    QVector<vtkIdType> m_lastBadCellIds;
    int m_lastTotalCellsForStats = 0;
    QString m_lastExportCsvPath;
    QString m_lastExportPngPath;
    QString m_lastMeshClassText;
    QString m_lastCellTypeStatsTableMd;
    QString m_lastMetricSupportSummary;
    QString m_lastImportPipelineText;

    void initVtkScene();
    void setComboLegendChromeVisible(bool visible);
    bool loadMesh(const QString& path);
    bool computeQuality();
    void updateVisualization();
    void updateBadCellsOverlay();
    void flushVtkLogToUi();
    void updateSelectedCellOverlay(vtkIdType cellId);
    void applyIsolationMode(bool enabled);
    void applyThresholdTemplate();
    bool ensureMetricComputed(const QString& metricKey);
    bool ensureMetricsComputedForRuleTable();

    // Async compute (avoid blocking UI for large meshes)
    QFutureWatcher<vtklasttry::ComputeResult>* m_computeWatcher = nullptr;
    QProgressDialog* m_computeProgress = nullptr;
    std::atomic<bool> m_cancelCompute{ false };
    bool m_lastComputeCanceled = false;

    using RuleCondition = vtklasttry::RuleCondition;

    void initRuleTableDefaults();
    void setupRuleTableRowWidgets(int row);
    void refreshRuleTableRowProperty();
    void fillMetricCombo(QComboBox* cb, const QString& selectKey = QString());
    void fillTemplateCombo(QComboBox* cb, const QString& selectKey = QString());
    QVector<RuleCondition> collectRuleConditionsFromTable() const;
    QString formatComboRuleText() const;
    void applyTemplateToRuleRow(int row);

    vtklasttry::SceneViewRequest buildSceneViewRequest() const;
    vtklasttry::OverlayRequest buildOverlayRequest() const;
    vtklasttry::ComputeRequest buildComputeRequest() const;
    vtklasttry::AppState buildComputeAppState();
    vtklasttry::QualityRequest buildQualityRequest() const;
    vtklasttry::QualityState buildQualityState();
    vtklasttry::MeshSelectRequest buildMeshSelectRequest() const;
    vtklasttry::MeshState buildMeshState();
};
