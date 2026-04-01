#include "ComputeWorkflow.h"

#include <QFutureWatcher>
#include <QCheckBox>
#include <QComboBox>
#include <QMessageBox>
#include <QMetaObject>
#include <QProgressDialog>
#include <QPushButton>
#include <QSet>
#include <QStatusBar>
#include <QWidget>
#include <QTextEdit>

#include <QtConcurrent/QtConcurrentRun>

#include <vtkDataArray.h>
#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkGenericOpenGLRenderWindow.h>

#include "QualityEngine.h"
#include "RuleEngine.h"
#include "UiLog.h"
#include "UiStateController.h"

namespace vtklasttry
{
namespace
{
static void onComputeAndShowCore(QObject* uiThreadObject,
                                 QObject* parentForDialogs,
                                 QTextEdit* logBox,
                                 QStatusBar* statusBar,
                                 QPushButton* computeBtn,
                                 QPushButton* importBtn,
                                 const QString& metricKey,
                                 bool comboEnabled,
                                 const QVector<vtklasttry::RuleCondition>& tableConds,
                                 vtklasttry::AppState& state,
                                 const std::function<void()>& updateVisualization,
                                 const std::function<void()>& updateBadCellsOverlay,
                                 const std::function<void()>& flushVtkLogToUi)
{
    if (!uiThreadObject || !parentForDialogs || !state.meshData || !state.qualityCache || !state.qualityArray ||
        !state.lastQualityMetricKey || !state.watcher || !state.progress || !state.cancelFlag || !state.lastComputeCanceled)
        return;

    if (!(*state.meshData))
    {
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u8BF7\u5148\u52A0\u8F7D\u7F51\u683C\uFF0C\u518D\u8BA1\u7B97\u8D28\u91CF\u3002"));
        QMessageBox::information(
            qobject_cast<QWidget*>(parentForDialogs),
            QString::fromWCharArray(L"\u63D0\u793A"),
            QString::fromWCharArray(L"\u8BF7\u5148\u9009\u62E9\u5E76\u52A0\u8F7D *.vtk\u3001*.vtu \u6216 *.stl \u6587\u4EF6\u3002"));
        return;
    }

    if (!(*state.watcher))
        *state.watcher = new QFutureWatcher<vtklasttry::ComputeResult>(qobject_cast<QObject*>(parentForDialogs));

    // If a previous compute is still running, ignore repeated clicks.
    if ((*state.watcher)->isRunning())
    {
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u6B63\u5728\u8BA1\u7B97\u4E2D\uFF0C\u8BF7\u7A0D\u540E\u2026"));
        return;
    }

    const bool canReuseQuality = (state.qualityCache->contains(metricKey) && state.qualityCache->value(metricKey));

    // Fast path: cached scalar already exists, do UI updates synchronously.
    if (!comboEnabled && canReuseQuality)
    {
        *state.qualityArray = state.qualityCache->value(metricKey);
        if ((*state.meshData) && (*state.meshData)->GetCellData())
            (*state.meshData)->GetCellData()->SetActiveScalars((*state.qualityArray)->GetName());
        *state.lastQualityMetricKey = metricKey;
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(
                                                L"\u590D\u7528\u5DF2\u8BA1\u7B97\u7684\u8D28\u91CF\u6570\u7EC4\uFF0C\u4EC5\u66F4\u65B0\u7B5B\u9009\u4E0E\u6E32\u67D3\u3002"));
        if (updateVisualization) updateVisualization();
        if (updateBadCellsOverlay) updateBadCellsOverlay();
        if (state.renderWindow)
            state.renderWindow->Render();
        if (flushVtkLogToUi) flushVtkLogToUi();
        return;
    }

    // Disable main actions while computing.
    vtklasttry::UiStateController::setComputing(computeBtn, importBtn, true);
    vtklasttry::UiStateController::showStatusComputing(statusBar, logBox);

    // Progress + cancel
    state.cancelFlag->store(false);
    *state.lastComputeCanceled = false;
    if (!(*state.progress))
    {
        *state.progress = new QProgressDialog(qobject_cast<QWidget*>(parentForDialogs));
        (*state.progress)->setWindowTitle(QString::fromWCharArray(L"\u8BA1\u7B97\u8FDB\u5EA6"));
        (*state.progress)->setCancelButtonText(QString::fromWCharArray(L"\u53D6\u6D88"));
        (*state.progress)->setMinimumDuration(200);
        (*state.progress)->setAutoClose(true);
        (*state.progress)->setAutoReset(true);
        (*state.progress)->setModal(true);
        QObject::connect(*state.progress, &QProgressDialog::canceled, uiThreadObject, [cancelFlag = state.cancelFlag]() {
            cancelFlag->store(true);
        });
    }

    // Compute in background; apply results in the GUI thread when finished.
    // IMPORTANT: do NOT capture `state` by reference in async callbacks.
    // `state` is typically a stack object in the caller and will be dangling when the future completes.
    auto watcherPtr = state.watcher;                // QFutureWatcher** (stable pointer provided by caller)
    auto progressPtr = state.progress;              // QProgressDialog** (stable pointer provided by caller)
    auto lastComputeCanceledPtr = state.lastComputeCanceled;
    auto meshDataPtr = state.meshData;
    auto qualityCachePtr = state.qualityCache;
    auto qualityArrayPtr = state.qualityArray;
    auto lastQualityMetricKeyPtr = state.lastQualityMetricKey;
    auto renderWindowSp = state.renderWindow;       // copy smart pointer

    QObject::disconnect(*state.watcher, nullptr, uiThreadObject, nullptr);
    QObject::connect(*state.watcher, &QFutureWatcher<vtklasttry::ComputeResult>::finished, uiThreadObject,
                     [=]() {
        if (!watcherPtr || !(*watcherPtr))
            return;
        const vtklasttry::ComputeResult res = (*watcherPtr)->result();
        if (progressPtr && *progressPtr)
        {
            (*progressPtr)->setValue((*progressPtr)->maximum());
            (*progressPtr)->hide();
        }
        vtklasttry::UiStateController::setComputing(computeBtn, importBtn, false);
        if (!res.ok)
        {
            if (res.canceled || (lastComputeCanceledPtr && *lastComputeCanceledPtr))
            {
                vtklasttry::UiStateController::showStatusComputeCanceled(statusBar, logBox);
            }
            else
            {
                vtklasttry::UiStateController::showStatusComputeFailed(statusBar, logBox);
            }
            if (flushVtkLogToUi) flushVtkLogToUi();
            return;
        }
        // Swap output on GUI thread (safe).
        if (res.meshData)
            *meshDataPtr = res.meshData;
        *qualityCachePtr = res.qualityCache;
        const QString mk = metricKey;
        if (qualityCachePtr && qualityCachePtr->contains(mk) && qualityCachePtr->value(mk))
        {
            *qualityArrayPtr = qualityCachePtr->value(mk);
            if ((*meshDataPtr) && (*meshDataPtr)->GetCellData())
                (*meshDataPtr)->GetCellData()->SetActiveScalars((*qualityArrayPtr)->GetName());
            if (lastQualityMetricKeyPtr)
                *lastQualityMetricKeyPtr = mk;
        }
        if (updateVisualization) updateVisualization();
        if (updateBadCellsOverlay) updateBadCellsOverlay();
        if (renderWindowSp)
            renderWindowSp->Render();
        if (flushVtkLogToUi) flushVtkLogToUi();
        vtklasttry::UiStateController::showStatusComputeDone(statusBar);
    });

    // Capture required inputs on GUI thread (do not touch widgets in worker).
    const QVector<vtklasttry::RuleCondition> conds = tableConds;
    QSet<QString> keys;
    if (comboEnabled)
    {
        for (const auto& c : conds)
            keys.insert(c.metricKey);
    }

    const int totalSteps = comboEnabled ? keys.size() : 1;
    if (*state.progress)
    {
        (*state.progress)->setRange(0, std::max(1, totalSteps));
        (*state.progress)->setValue(0);
        (*state.progress)->setLabelText(QString::fromWCharArray(L"\u6B63\u5728\u51C6\u5907\u8BA1\u7B97\u2026"));
        (*state.progress)->show();
    }

    vtkDataSet* sourceDs = vtkDataSet::SafeDownCast((*state.meshData));
    const std::atomic<bool>* cancelPtr = state.cancelFlag;
    QProgressDialog** inOutProgress = state.progress;
    (*state.watcher)->setFuture(QtConcurrent::run([=]() -> vtklasttry::ComputeResult {
        auto guiProgress = [&](int step, const QString& text) {
            if (!inOutProgress || !(*inOutProgress))
                return;
            QMetaObject::invokeMethod(*inOutProgress, "setLabelText", Qt::QueuedConnection, Q_ARG(QString, text));
            QMetaObject::invokeMethod(*inOutProgress, "setValue", Qt::QueuedConnection, Q_ARG(int, step));
        };

        if (!comboEnabled)
        {
            guiProgress(0, QString::fromWCharArray(L"\u6B63\u5728\u8BA1\u7B97\uFF1A%1").arg(metricKey));
            return vtklasttry::computeQualityOnCopy(sourceDs, QStringList() << metricKey, cancelPtr);
        }

        // Stable order for UI progress.
        QStringList keyList = QStringList(keys.values());
        std::sort(keyList.begin(), keyList.end());
        for (int i = 0; i < keyList.size(); ++i)
        {
            if (cancelPtr && cancelPtr->load())
            {
                vtklasttry::ComputeResult r;
                r.canceled = true;
                return r;
            }
            const QString k = keyList[i];
            guiProgress(i, QString::fromWCharArray(L"\u6B63\u5728\u8BA1\u7B97\uFF08%1/%2\uFF09\uFF1A%3").arg(i + 1).arg(keyList.size()).arg(k));
            vtklasttry::ComputeResult one = vtklasttry::computeQualityOnCopy(sourceDs, QStringList() << k, cancelPtr);
            if (!one.ok)
                return one;
            guiProgress(i + 1, QString::fromWCharArray(L"\u6B63\u5728\u8BA1\u7B97\uFF08%1/%2\uFF09\uFF1A%3").arg(i + 1).arg(keyList.size()).arg(k));
        }
        // Compute all keys in one pass (single copy) for combo mode.
        return vtklasttry::computeQualityOnCopy(sourceDs, keyList, cancelPtr);
    }));
}
} // namespace

void ComputeWorkflow::onComputeAndShow(QObject* uiThreadObject,
                                      QObject* parentForDialogs,
                                      QTextEdit* logBox,
                                      QStatusBar* statusBar,
                                      QPushButton* computeBtn,
                                      QPushButton* importBtn,
                                      const vtklasttry::ComputeRequest& req,
                                      vtklasttry::AppState& state,
                                      const std::function<void()>& updateVisualization,
                                      const std::function<void()>& updateBadCellsOverlay,
                                      const std::function<void()>& flushVtkLogToUi)
{
    onComputeAndShowCore(uiThreadObject,
                         parentForDialogs,
                         logBox,
                         statusBar,
                         computeBtn,
                         importBtn,
                         req.primaryMetricKey,
                         req.comboEnabled,
                         req.tableConds,
                         state,
                         updateVisualization,
                         updateBadCellsOverlay,
                         flushVtkLogToUi);
}
} // namespace vtklasttry

