#pragma once

#include "OverlayRequest.h"
#include "SceneViewRequest.h"

#include <QPair>
#include <QVector>

class mainwindow;

namespace vtklasttry
{
struct RuleCondition;
struct BadCellEvalResult;

class SceneController
{
public:
    static void initVtkScene(mainwindow* w);
    static void updateVisualization(mainwindow* w, const vtklasttry::SceneViewRequest& req);
    static void updateBadCellsOverlay(mainwindow* w,
                                      const vtklasttry::OverlayRequest& req);
    // Step 1 refactor: rendering-only part of bad-cells overlay (no UI list updates).
    static void updateBadCellsOverlayRender(mainwindow* w,
                                           bool comboEnabled,
                                           const QVector<vtklasttry::RuleCondition>& tableConds,
                                           const vtklasttry::BadCellEvalResult& eval,
                                           const QVector<QPair<QString, QString>>& metricUiItems);
    static void updateSelectedCellOverlay(mainwindow* w, long long cellId);
    static void applyIsolationMode(mainwindow* w, bool enabled, const vtklasttry::SceneViewRequest& req);
    static void pickFromInteractor(mainwindow* w, const vtklasttry::SceneViewRequest& req);
};
} // namespace vtklasttry

