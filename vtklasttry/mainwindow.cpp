
#include "mainwindow.h"

#include <array>
#include <vector>

#include "OverlayRequest.h"

// Qt
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QSaveFile>
#include <QHash>
#include <QTableWidget>
#include <QHeaderView>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QAbstractItemView>
#include <QSet>
#include <algorithm>
#include <limits>
#include <cmath>
#include <QTextCodec>
#include <map>
#include <cmath>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QStringList>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFrame>
#include <QWidgetAction>

// VTK initialization

#include "SceneViewRequest.h"
#include "OverlayRequest.h"
#include "ComputeRequest.h"
#include "AppState.h"
#include "QualityRequest.h"
#include "QualityState.h"
#include "MeshSelectRequest.h"
#include "MeshState.h"
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

// VTK headers
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkCellData.h>
#include <vtkDataSetMapper.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkGeometryFilter.h>
#include <vtkDataObject.h>
#include <vtkLookupTable.h>
#include <vtkMeshQuality.h>
#include <vtkOutputWindow.h>
#include <vtkStringOutputWindow.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>
#include <vtkLegendBoxActor.h>
#include <vtkPlaneSource.h>
#include <vtkTextActor.h>
#include <vtkThreshold.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkDataSet.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkDataSetReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkIdList.h>
#include <vtkSTLReader.h>
#include <vtkExtractCells.h>
#include <vtkIdTypeArray.h>
#include <vtkOutlineFilter.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
#include <vtkQuad.h>
#include <vtkCellTypes.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkCell.h>
#include <vtkCell3D.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCellPicker.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>

#include "MeshIO.h"
#include "UiLog.h"
#include "ExportEngine.h"
#include "QualityEngine.h"
#include "RuleEngine.h"
#include "SceneController.h"
#include "BadCellsPanel.h"
#include "RuleTablePanel.h"
#include "QualitySupport.h"
#include "MeshWorkflow.h"
#include "ComputeWorkflow.h"
#include "QualityWorkflow.h"
#include "MeshAnalysis.h"
#include "VtkLogCapture.h"
#include "ThresholdTemplates.h"
#include "MenuController.h"
#include "TemplateController.h"

// (cleanup) legacy helpers migrated to modules; keep mainwindow as shell/state container.

mainwindow::mainwindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    setWindowTitle(QString::fromWCharArray(L"\u8239\u8236CFD\u7F51\u683C\u8D28\u91CF\u8BC4\u4F30\u4E0E\u53EF\u89C6\u5316"));

    vtklasttry::MenuController::setupMenus(this,
                                           menuBar(),
                                           ui.dockBadCells,
                                           ui.dockLog,
                                           this,
                                           SLOT(onSelectFile()),
                                           this,
                                           SLOT(onExportBadCellsCsv()),
                                           this,
                                           SLOT(onExportScreenshotPng()),
                                           this,
                                           SLOT(onExportReportMd()));

    // Add black borders for docks and their inner widgets.
    if (ui.dockLog)
        ui.dockLog->setStyleSheet(QStringLiteral("QDockWidget{border:1px solid #000000;} QDockWidget::title{padding:2px;}"));
    if (ui.dockBadCells)
        ui.dockBadCells->setStyleSheet(QStringLiteral("QDockWidget{border:1px solid #000000;} QDockWidget::title{padding:2px;}"));
    if (ui.textLog)
        ui.textLog->setStyleSheet(QStringLiteral("QTextEdit{border:1px solid #000000;}"));
    if (ui.listBadCells)
        ui.listBadCells->setStyleSheet(QStringLiteral("QListWidget{border:1px solid #000000;}"));
    // UI init
    ui.comboMetric->addItem(QString::fromWCharArray(L"Aspect Ratio\uFF08\u957F\u5BBD\u6BD4\uFF09"), "aspect");
    ui.comboMetric->addItem(QString::fromWCharArray(L"Skew\uFF08\u626D\u66F2\u5EA6\uFF09"), "skew");
    ui.comboMetric->addItem(QString::fromWCharArray(L"Scaled Jacobian\uFF08\u7F29\u653E\u96C5\u53EF\u6BD4\uFF09"), "scaled_jacobian");
    ui.comboMetric->addItem(QString::fromWCharArray(L"Jacobian\uFF08\u96C5\u53EF\u6BD4\uFF0C\u4E09\u89D2\u7528\u7F29\u653E\u96C5\u53EF\u6BD4\u4EE3\u66FF\uFF09"), "jacobian");
    ui.comboMetric->addItem(QString::fromWCharArray(L"Jacobian Ratio\uFF08\u8FB9\u957F\u6BD4 max/min\uFF09"), "jacobian_ratio");
    ui.comboMetric->addItem(QString::fromWCharArray(L"Min Angle\uFF08\u6700\u5C0F\u89D2\uFF0C\u5EA6\uFF09"), "min_angle");
    ui.comboMetric->addItem(QString::fromWCharArray(L"Max Angle\uFF08\u6700\u5927\u89D2\uFF0C\u5EA6\uFF09"), "max_angle");
    ui.comboMetric->addItem(QString::fromWCharArray(L"Min Edge Length\uFF08\u6700\u5C0F\u8FB9\u957F\uFF09"), "min_edge_length");
    ui.comboMetric->addItem(QString::fromWCharArray(L"Non-orthogonality\uFF08\u975E\u6B63\u4EA4\u5EA6\uFF0C\u5EA6\uFF09"), "non_ortho");

    ui.comboBadRule->addItem(QString::fromWCharArray(L"\u574F\uFF1Aq \u2265 \u9608\u503C"), "ge");
    ui.comboBadRule->addItem(QString::fromWCharArray(L"\u574F\uFF1Aq \u2264 \u9608\u503C"), "le");

    ui.comboLogicOp->addItem(QString::fromWCharArray(L"\u4E14\uFF08AND\uFF09"), "and");
    ui.comboLogicOp->addItem(QString::fromWCharArray(L"\u6216\uFF08OR\uFF09"), "or");

    ui.tableRuleConditions->setHorizontalHeaderLabels(QStringList()
                                                        << QString::fromWCharArray(L"\u6307\u6807")
                                                        << QString::fromWCharArray(L"\u5224\u5B9A")
                                                        << QString::fromWCharArray(L"\u9608\u503C")
                                                        << QString::fromWCharArray(L"\u6A21\u677F"));
    ui.tableRuleConditions->verticalHeader()->setVisible(true);
    ui.tableRuleConditions->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.tableRuleConditions->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.tableRuleConditions->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui.comboTemplate->addItem(QString::fromWCharArray(L"\u81EA\u5B9A\u4E49"), "custom");
    ui.comboTemplate->addItem(QString::fromWCharArray(L"\u5E38\u7528\u6807\u51C6\uFF08\u4E00\u822C\uFF09"), "std_normal");
    ui.comboTemplate->addItem(QString::fromWCharArray(L"\u5E38\u7528\u6807\u51C6\uFF08\u4E25\u683C\uFF09"), "std_strict");
    ui.comboTemplate->addItem(QString::fromWCharArray(L"\u5E38\u7528\u6807\u51C6\uFF08\u5BBD\u677E\uFF09"), "std_loose");
    ui.comboTemplate->addItem(QString::fromWCharArray(L"\u81EA\u9002\u5E94\uFF08\u5206\u4F4D\u6570\uFF09"), "adaptive");

    initRuleTableDefaults();

    connect(ui.pushButton_2, &QPushButton::clicked, this, &mainwindow::onSelectFile);
    connect(ui.pushButton, &QPushButton::clicked, this, &mainwindow::onComputeAndShow);
    // ???????????????????????????????????????????????????
    connect(ui.comboMetric, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mainwindow::onMetricOrThresholdChanged);
    connect(ui.spinThreshold, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &mainwindow::onMetricOrThresholdChanged);
    connect(ui.checkShowBad, &QCheckBox::toggled, this, &mainwindow::onMetricOrThresholdChanged);
    connect(ui.comboBadRule, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mainwindow::onMetricOrThresholdChanged);
    connect(ui.checkEnableComboRule, &QCheckBox::toggled, this, &mainwindow::onComboRuleToggled);
    connect(ui.comboLogicOp, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mainwindow::onMetricOrThresholdChanged);
    connect(ui.btnAddRuleRow, &QPushButton::clicked, this, &mainwindow::onAddRuleRow);
    connect(ui.btnRemoveRuleRow, &QPushButton::clicked, this, &mainwindow::onRemoveRuleRow);
    connect(ui.listBadCells, &QListWidget::itemActivated, this, &mainwindow::onBadCellActivated);
    connect(ui.btnExportBadCsv, &QPushButton::clicked, this, &mainwindow::onExportBadCellsCsv);
    connect(ui.btnExportScreenshot, &QPushButton::clicked, this, &mainwindow::onExportScreenshotPng);
    connect(ui.btnExportReportMd, &QPushButton::clicked, this, &mainwindow::onExportReportMd);
    connect(ui.checkIsolateSelectedBad, &QCheckBox::toggled, this, &mainwindow::onIsolateSelectedBadToggled);
    connect(ui.comboTemplate, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mainwindow::onTemplateChanged);

    // Capture VTK log into our Qt panel (avoid vtkOutputWindow popup).
    m_vtkLog = vtkSmartPointer<vtkStringOutputWindow>::New();
    vtkOutputWindow::SetInstance(m_vtkLog);

    initVtkScene();
    statusBar()->showMessage(QString::fromWCharArray(L"\u5C31\u7EEA\uFF1A\u8BF7\u9009\u62E9 *.vtk\uFF08\u4F18\u5148\uFF09\u6216 *.vtu \u7F51\u683C\u6587\u4EF6"));

    onComboRuleToggled(ui.checkEnableComboRule->isChecked());
}

mainwindow::~mainwindow()
{
    if (m_computeWatcher)
    {
        m_computeWatcher->cancel();
        m_computeWatcher->waitForFinished();
        delete m_computeWatcher;
        m_computeWatcher = nullptr;
    }
    if (m_computeProgress)
    {
        m_computeProgress->close();
        delete m_computeProgress;
        m_computeProgress = nullptr;
    }
}

void mainwindow::onComboRuleToggled(bool checked)
{
    vtklasttry::RuleTablePanel::onComboRuleToggled(checked,
                                                   ui.comboMetric,
                                                   ui.comboBadRule,
                                                   ui.spinThreshold,
                                                   ui.labelMetric,
                                                   ui.labelThreshold,
                                                   ui.labelBadRule,
                                                   ui.comboLogicOp,
                                                   ui.labelLogicOp,
                                                   ui.labelRuleTable,
                                                   ui.tableRuleConditions,
                                                   ui.btnAddRuleRow,
                                                   ui.btnRemoveRuleRow,
                                                   ui.labelTemplate,
                                                   ui.comboTemplate,
                                                   statusBar(),
                                                   this,
                                                   "onMetricOrThresholdChanged",
                                                   "onAnyRuleTemplateChanged");
}

void mainwindow::fillMetricCombo(QComboBox* cb, const QString& selectKey)
{
    vtklasttry::RuleTablePanel::fillMetricCombo(ui.comboMetric, cb, selectKey);
}

void mainwindow::fillTemplateCombo(QComboBox* cb, const QString& selectKey)
{
    vtklasttry::RuleTablePanel::fillTemplateCombo(ui.comboTemplate, cb, selectKey);
}

void mainwindow::initRuleTableDefaults()
{
    vtklasttry::RuleTablePanel::initRuleTableDefaults(ui.tableRuleConditions,
                                                      ui.comboMetric,
                                                      ui.comboBadRule,
                                                      ui.spinThreshold,
                                                      ui.comboTemplate,
                                                      this,
                                                      "onMetricOrThresholdChanged",
                                                      "onAnyRuleTemplateChanged");
}

void mainwindow::setupRuleTableRowWidgets(int row)
{
    vtklasttry::RuleTablePanel::setupRuleTableRowWidgets(ui.tableRuleConditions,
                                                         row,
                                                         ui.comboMetric,
                                                         ui.comboBadRule,
                                                         ui.spinThreshold,
                                                         ui.comboTemplate,
                                                         this,
                                                         "onMetricOrThresholdChanged",
                                                         "onAnyRuleTemplateChanged");
}

void mainwindow::refreshRuleTableRowProperty()
{
    vtklasttry::RuleTablePanel::refreshRuleTableRowProperty(ui.tableRuleConditions);
}

void mainwindow::onAddRuleRow()
{
    vtklasttry::RuleTablePanel::onAddRuleRow(ui.tableRuleConditions,
                                             ui.comboMetric,
                                             ui.comboBadRule,
                                             ui.spinThreshold,
                                             ui.comboTemplate,
                                             statusBar(),
                                             ui.checkEnableComboRule->isChecked(),
                                             this,
                                             "onMetricOrThresholdChanged",
                                             "onAnyRuleTemplateChanged");
}

void mainwindow::onRemoveRuleRow()
{
    vtklasttry::RuleTablePanel::onRemoveRuleRow(ui.tableRuleConditions,
                                                statusBar(),
                                                ui.checkEnableComboRule->isChecked(),
                                                this);
}

void mainwindow::onAnyRuleTemplateChanged()
{
    vtklasttry::RuleTablePanel::onAnyRuleTemplateChanged(sender(),
                                                         ui.tableRuleConditions,
                                                         statusBar(),
                                                         m_meshData,
                                                         m_qualityCache);
}

void mainwindow::applyTemplateToRuleRow(int row)
{
    vtklasttry::RuleTablePanel::applyTemplateToRuleRow(ui.tableRuleConditions, row, m_meshData, m_qualityCache);
}

QVector<mainwindow::RuleCondition> mainwindow::collectRuleConditionsFromTable() const
{
    return vtklasttry::collectRuleConditionsFromTable(ui.tableRuleConditions);
}

QString mainwindow::formatComboRuleText() const
{
    const QVector<vtklasttry::RuleCondition> conds = vtklasttry::collectRuleConditionsFromTable(ui.tableRuleConditions);
    return vtklasttry::formatComboRuleText(conds, ui.comboLogicOp->currentData().toString());
}

bool mainwindow::ensureMetricsComputedForRuleTable()
{
    const auto conds = vtklasttry::collectRuleConditionsFromTable(ui.tableRuleConditions);
    QSet<QString> keys;
    for (const auto& c : conds)
        keys.insert(c.metricKey);
    for (const QString& k : keys)
    {
        if (!ensureMetricComputed(k))
            return false;
    }
    return true;
}

namespace
{
    static void appendUiLog(QTextEdit* box, const QString& msg) { vtklasttry::appendUiLog(box, msg); }
}

// onLeftButtonPressPick moved to SceneController.cpp

void mainwindow::pickFromInteractor()
{
    vtklasttry::SceneViewRequest req = buildSceneViewRequest();
    vtklasttry::SceneController::pickFromInteractor(this, req);
}

void mainwindow::initVtkScene()
{
    vtklasttry::SceneController::initVtkScene(this);
}

void mainwindow::setComboLegendChromeVisible(bool visible)
{
    if (m_comboLegend)
        m_comboLegend->SetVisibility(visible);
    if (m_comboLegendTitle)
        m_comboLegendTitle->SetVisibility(visible);
}

vtklasttry::SceneViewRequest mainwindow::buildSceneViewRequest() const
{
    vtklasttry::SceneViewRequest req;
    req.metricKey = ui.comboMetric->currentData().toString();
    req.comboEnabled = ui.checkEnableComboRule->isChecked();
    req.showBad = ui.checkShowBad->isChecked();
    req.isolateSelectedBad = ui.checkIsolateSelectedBad->isChecked();
    return req;
}

vtklasttry::OverlayRequest mainwindow::buildOverlayRequest() const
{
    vtklasttry::OverlayRequest req;
    req.showBad = ui.checkShowBad->isChecked();
    req.comboEnabled = ui.checkEnableComboRule->isChecked();
    req.metricKeySingle = ui.comboMetric->currentData().toString();
    req.thresholdSingle = ui.spinThreshold->value();
    req.isGESingle = (ui.comboBadRule->currentData().toString() == "ge");
    req.logicOp = ui.comboLogicOp->currentData().toString();
    req.tableConds = collectRuleConditionsFromTable();
    req.comboRuleText = formatComboRuleText();
    req.metricUiItems.reserve(ui.comboMetric->count());
    for (int i = 0; i < ui.comboMetric->count(); ++i)
    {
        req.metricUiItems.push_back(qMakePair(ui.comboMetric->itemData(i).toString(),
                                              ui.comboMetric->itemText(i)));
    }
    return req;
}

vtklasttry::ComputeRequest mainwindow::buildComputeRequest() const
{
    vtklasttry::ComputeRequest req;
    req.primaryMetricKey = ui.comboMetric->currentData().toString();
    req.comboEnabled = ui.checkEnableComboRule->isChecked();
    req.logicOp = ui.comboLogicOp->currentData().toString();
    req.tableConds = collectRuleConditionsFromTable();
    return req;
}

vtklasttry::AppState mainwindow::buildComputeAppState()
{
    vtklasttry::AppState state;
    state.renderWindow = m_renderWindow;
    state.meshData = &m_meshData;
    state.qualityCache = &m_qualityCache;
    state.qualityArray = &m_qualityArray;
    state.lastQualityMetricKey = &m_lastQualityMetricKey;
    state.watcher = &m_computeWatcher;
    state.progress = &m_computeProgress;
    state.cancelFlag = &m_cancelCompute;
    state.lastComputeCanceled = &m_lastComputeCanceled;
    return state;
}

vtklasttry::QualityRequest mainwindow::buildQualityRequest() const
{
    vtklasttry::QualityRequest req;
    req.metricKey = ui.comboMetric->currentData().toString();
    req.thresholdValue = ui.spinThreshold->value();
    req.templateKey = ui.comboTemplate->currentData().toString();
    return req;
}

vtklasttry::QualityState mainwindow::buildQualityState()
{
    vtklasttry::QualityState state;
    state.scalarBar = m_scalarBar;
    state.meshData = &m_meshData;
    state.qualityArray = &m_qualityArray;
    state.qualityCache = &m_qualityCache;
    state.lastQualityMetricKey = &m_lastQualityMetricKey;
    state.lastMetricSupportSummary = &m_lastMetricSupportSummary;
    return state;
}

vtklasttry::MeshSelectRequest mainwindow::buildMeshSelectRequest() const
{
    vtklasttry::MeshSelectRequest req;
    req.openDialogTitle = QString::fromWCharArray(L"\u9009\u62E9\u7F51\u683C\u6587\u4EF6");
    return req;
}

vtklasttry::MeshState mainwindow::buildMeshState()
{
    vtklasttry::MeshState state;
    state.renderer = m_renderer;
    state.renderWindow = m_renderWindow;
    state.currentFilePath = &m_currentFilePath;
    state.lastImportPipelineText = &m_lastImportPipelineText;
    state.meshData = &m_meshData;
    state.qualityArray = &m_qualityArray;
    state.lastQualityMetricKey = &m_lastQualityMetricKey;
    state.qualityCache = &m_qualityCache;
    state.selectedCellId = &m_selectedCellId;
    state.isolatedActor = m_isolatedActor;
    state.lastBadCellIds = &m_lastBadCellIds;
    state.lastTotalCellsForStats = &m_lastTotalCellsForStats;
    state.badStatsLabel = ui.labelBadStats;
    state.lastMeshClassText = &m_lastMeshClassText;
    state.lastCellTypeStatsTableMd = &m_lastCellTypeStatsTableMd;
    state.meshActor = &m_meshActor;
    state.scalarBar = m_scalarBar;
    state.comboLegend = m_comboLegend;
    state.comboLegendTitle = m_comboLegendTitle;
    return state;
}

void mainwindow::onSelectFile()
{
    vtklasttry::MeshSelectRequest req = buildMeshSelectRequest();
    vtklasttry::MeshState state = buildMeshState();

    vtklasttry::MeshWorkflow::onSelectFile(this,
                                           ui.textLog,
                                           statusBar(),
                                           req,
                                           state);
    flushVtkLogToUi();
}

void mainwindow::onComputeAndShow()
{
    vtklasttry::ComputeRequest req = buildComputeRequest();
    vtklasttry::AppState state = buildComputeAppState();

    vtklasttry::ComputeWorkflow::onComputeAndShow(this,
                                                 this,
                                                 ui.textLog,
                                                 statusBar(),
                                                 ui.pushButton,
                                                 ui.pushButton_2,
                                                 req,
                                                 state,
                                                 [this]() { updateVisualization(); },
                                                 [this]() { updateBadCellsOverlay(); },
                                                 [this]() { flushVtkLogToUi(); });
}

void mainwindow::onMetricOrThresholdChanged()
{
    if (!m_meshData)
        return;

    // ??????????????????????????????????????????????
    statusBar()->showMessage(QString::fromWCharArray(
        L"\u53C2\u6570\u5DF2\u4FEE\u6539\uFF0C\u8BF7\u70B9\u51FB\u201C\u8BA1\u7B97\u5E76\u663E\u793A\u201D\u66F4\u65B0\u7ED3\u679C\u3002"));
}

bool mainwindow::ensureMetricComputed(const QString& metricKey)
{
    if (!m_meshData)
        return false;
    if (metricKey.isEmpty())
        return false;
    if (m_qualityCache.contains(metricKey) && m_qualityCache.value(metricKey))
        return true;

    appendUiLog(ui.textLog, QString::fromWCharArray(L"\u7EC4\u5408\u7B5B\u9009\u9700\u8981\u8BA1\u7B97\u6307\u6807\uFF1A%1").arg(metricKey));
    vtklasttry::QualityRequest req = buildQualityRequest();
    req.metricKey = metricKey; // override UI primary metric; avoid mutating widget state
    vtklasttry::QualityState state = buildQualityState();
    const bool ok = vtklasttry::QualityWorkflow::computeQuality(this,
                                                                ui.textLog,
                                                                statusBar(),
                                                                req,
                                                                state,
                                                                [](const QString&) {},
                                                                [](double) {},
                                                                []() {});

    // Restore active scalars to primary metric cache if present.
    const QString primaryKey = ui.comboMetric->currentData().toString();
    if (m_qualityCache.contains(primaryKey) && m_qualityCache.value(primaryKey))
    {
        m_qualityArray = m_qualityCache.value(primaryKey);
        if (m_meshData && m_meshData->GetCellData())
            m_meshData->GetCellData()->SetActiveScalars(m_qualityArray->GetName());
        m_lastQualityMetricKey = primaryKey;
    }
    return ok;
}

void mainwindow::flushVtkLogToUi()
{
    vtklasttry::flushVtkLogToUi(ui.textLog, m_vtkLog);
}

bool mainwindow::loadMesh(const QString& path)
{
    QString pipeline;
    vtkSmartPointer<vtkDataSet> ds;
    QString meshClassText;
    QString statsMd;
    const bool ok = vtklasttry::MeshWorkflow::loadMesh(this,
                                                       ui.textLog,
                                                       statusBar(),
                                                       m_renderer,
                                                       m_renderWindow,
                                                       path,
                                                       &pipeline,
                                                       &ds,
                                                       &meshClassText,
                                                       &statsMd,
                                                       &m_meshActor,
                                                       m_scalarBar,
                                                       m_comboLegend,
                                                       m_comboLegendTitle);
    if (!ok)
        return false;

    m_currentFilePath = path;
    m_lastImportPipelineText = pipeline;
    m_meshData = ds;
    m_qualityArray = nullptr;
    m_lastQualityMetricKey.clear();
    m_qualityCache.clear();
    m_selectedCellId = -1;
    if (m_isolatedActor)
        m_isolatedActor->SetVisibility(false);
    m_lastBadCellIds.clear();
    m_lastTotalCellsForStats = static_cast<int>(ds ? ds->GetNumberOfCells() : 0);
    if (ui.labelBadStats)
        ui.labelBadStats->setText(QString::fromWCharArray(L"\u574F\u5355\u5143\u7EDF\u8BA1\uFF1A-"));
    m_lastMeshClassText = meshClassText;
    m_lastCellTypeStatsTableMd = statsMd;
    return true;
}

bool mainwindow::computeQuality()
{
    vtklasttry::QualityRequest req = buildQualityRequest();
    vtklasttry::QualityState state = buildQualityState();

    return vtklasttry::QualityWorkflow::computeQuality(this,
                                                       ui.textLog,
                                                       statusBar(),
                                                       req,
                                                       state,
                                                       [this](const QString& key) {
                                                           ui.comboBadRule->setCurrentIndex(ui.comboBadRule->findData(key));
                                                       },
                                                       [this](double v) {
                                                           ui.spinThreshold->setValue(v);
                                                       },
                                                       [this]() {
                                                           if (ui.comboTemplate->currentData().toString() != "custom")
                                                               applyThresholdTemplate();
                                                       });
}

void mainwindow::updateVisualization()
{
    vtklasttry::SceneViewRequest req = buildSceneViewRequest();
    vtklasttry::SceneController::updateVisualization(this, req);
}

void mainwindow::updateBadCellsOverlay()
{
    vtklasttry::OverlayRequest req = buildOverlayRequest();
    vtklasttry::SceneController::updateBadCellsOverlay(this, req);
}

void mainwindow::onTemplateChanged(int)
{
    applyThresholdTemplate();
    statusBar()->showMessage(QString::fromWCharArray(
        L"\u6A21\u677F\u5DF2\u5E94\u7528\uFF0C\u8BF7\u70B9\u51FB\u201C\u8BA1\u7B97\u5E76\u663E\u793A\u201D\u66F4\u65B0\u7ED3\u679C\u3002"));
}

void mainwindow::applyThresholdTemplate()
{
    vtklasttry::TemplateController::applyThresholdTemplate(ui.checkEnableComboRule,
                                                           ui.comboTemplate,
                                                           ui.comboMetric,
                                                           ui.comboBadRule,
                                                           ui.spinThreshold,
                                                           ui.textLog,
                                                           m_meshData,
                                                           m_qualityArray,
                                                           m_qualityCache);
}

void mainwindow::onExportBadCellsCsv()
{
    vtklasttry::MenuController::onExportBadCellsCsv(this,
                                                    ui.textLog,
                                                    ui.comboMetric,
                                                    ui.comboBadRule,
                                                    ui.spinThreshold,
                                                    ui.checkEnableComboRule,
                                                    ui.comboLogicOp,
                                                    ui.tableRuleConditions,
                                                    m_currentFilePath,
                                                    formatComboRuleText(),
                                                    m_meshData,
                                                    m_qualityArray,
                                                    m_qualityCache,
                                                    m_lastBadCellIds,
                                                    &m_lastExportCsvPath);
}

void mainwindow::onExportScreenshotPng()
{
    vtklasttry::MenuController::onExportScreenshotPng(this, ui.textLog, m_renderWindow, &m_lastExportPngPath);
}

void mainwindow::onExportReportMd()
{
    QString savedPath;
    vtklasttry::MenuController::onExportReportMd(this,
                                                 ui.textLog,
                                                 ui.comboMetric,
                                                 ui.comboBadRule,
                                                 ui.comboTemplate,
                                                 ui.spinThreshold,
                                                 ui.checkEnableComboRule,
                                                 m_currentFilePath,
                                                 m_lastImportPipelineText,
                                                 m_lastMeshClassText,
                                                 m_lastCellTypeStatsTableMd,
                                                 m_lastMetricSupportSummary,
                                                 formatComboRuleText(),
                                                 m_lastExportCsvPath,
                                                 m_lastExportPngPath,
                                                 m_meshData,
                                                 m_qualityArray,
                                                 m_lastBadCellIds,
                                                 m_lastTotalCellsForStats,
                                                 &savedPath);
}

void mainwindow::onBadCellActivated(QListWidgetItem* item)
{
    if (!item)
        return;
    bool ok = false;
    const qlonglong cellId = item->data(Qt::UserRole).toLongLong(&ok);
    if (!ok)
        return;
    statusBar()->showMessage(QString::fromWCharArray(L"\u9009\u4E2D\u574F\u5355\u5143\uFF1AcellId=%1").arg(cellId));
    m_selectedCellId = static_cast<vtkIdType>(cellId);
    updateSelectedCellOverlay(m_selectedCellId);
    if (ui.checkIsolateSelectedBad->isChecked())
        applyIsolationMode(true);
    m_renderWindow->Render();
}

void mainwindow::onIsolateSelectedBadToggled(bool checked)
{
    applyIsolationMode(checked);
    m_renderWindow->Render();
}

void mainwindow::applyIsolationMode(bool enabled)
{
    vtklasttry::SceneViewRequest req = buildSceneViewRequest();
    vtklasttry::SceneController::applyIsolationMode(this, enabled, req);
}

void mainwindow::updateSelectedCellOverlay(vtkIdType cellId)
{
    vtklasttry::SceneController::updateSelectedCellOverlay(this, static_cast<long long>(cellId));
}


