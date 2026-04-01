#include "SceneController.h"

#include <array>

#include <QFileInfo>

#include <vtkCallbackCommand.h>
#include <vtkCellPicker.h>
#include <vtkCommand.h>
#include <vtkCellTypes.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkLegendBoxActor.h>
#include <vtkLookupTable.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkExtractCells.h>
#include <vtkDataSetMapper.h>
#include <vtkIdList.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkCamera.h>
#include <vtkGeometryFilter.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkPlaneSource.h>

#include "mainwindow.h"
#include "UiLog.h"
#include "RuleEngine.h"
#include "BadCellsPanel.h"
#include "SelectionController.h"
#include "PickController.h"
#include "MetricNameResolver.h"

namespace
{
static const std::vector<std::array<double, 3>>& pathologicLegendColors()
{
    static const std::vector<std::array<double, 3>> k = {
        { {1.00, 0.90, 0.15} }, // yellow
        { {1.00, 0.55, 0.12} }, // orange
        { {0.05, 0.48, 0.48} }, // dark teal
        { {1.00, 0.45, 0.42} }, // coral
        { {1.00, 0.00, 0.72} }, // magenta
        { {1.00, 0.75, 0.82} }, // light pink
        { {0.55, 0.35, 0.18} }, // brown
        { {0.32, 0.28, 0.78} }, // purple-blue
        { {0.15, 0.82, 0.90} }, // cyan
        { {0.58, 0.22, 0.72} }, // purple
    };
    return k;
}

static QString pickChineseFontFile()
{
    const QStringList candidates = {
        QStringLiteral("C:/Windows/Fonts/msyh.ttc"),
        QStringLiteral("C:/Windows/Fonts/msyh.ttf"),
        QStringLiteral("C:/Windows/Fonts/msyhl.ttc"),
        QStringLiteral("C:/Windows/Fonts/simhei.ttf"),
        QStringLiteral("C:/Windows/Fonts/simsun.ttc"),
        QStringLiteral("C:/Windows/Fonts/simsun.ttf"),
        QStringLiteral("C:/Windows/Fonts/arialuni.ttf")
    };
    for (const QString& p : candidates)
    {
        if (QFileInfo::exists(p))
            return p;
    }
    return QString();
}

static void onLeftButtonPressPick(vtkObject*, unsigned long, void* clientData, void*)
{
    mainwindow* self = reinterpret_cast<mainwindow*>(clientData);
    if (!self)
        return;
    self->pickFromInteractor();
}
} // namespace

namespace vtklasttry
{
void SceneController::initVtkScene(mainwindow* w)
{
    if (!w)
        return;

    vtkNew<vtkNamedColors> colors;
    std::array<unsigned char, 4> bkg{ { 26, 51, 102, 255 } };
    colors->SetColor("BkgColor", bkg[0], bkg[1], bkg[2], bkg[3]);

    w->rendererSp() = vtkSmartPointer<vtkRenderer>::New();
    w->rendererSp()->SetBackground(colors->GetColor3d("BkgColor").GetData());

    w->renderWindowSp() = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    w->renderWindowSp()->AddRenderer(w->rendererSp());
    w->vtkWidget()->SetRenderWindow(w->renderWindowSp());

    w->cellPickerSp() = vtkSmartPointer<vtkCellPicker>::New();
    w->cellPickerSp()->SetTolerance(0.0005);
    w->pickCallbackSp() = vtkSmartPointer<vtkCallbackCommand>::New();
    w->pickCallbackSp()->SetCallback(onLeftButtonPressPick);
    w->pickCallbackSp()->SetClientData(w);
    if (w->vtkWidget() && w->vtkWidget()->GetInteractor())
        w->vtkWidget()->GetInteractor()->AddObserver(vtkCommand::LeftButtonPressEvent, w->pickCallbackSp(), 0.0);

    w->lutSp() = vtkSmartPointer<vtkLookupTable>::New();
    w->lutSp()->SetNumberOfTableValues(256);
    w->lutSp()->SetHueRange(0.667, 0.0);
    w->lutSp()->Build();

    w->scalarBarSp() = vtkSmartPointer<vtkScalarBarActor>::New();
    w->scalarBarSp()->SetLookupTable(w->lutSp());
    w->scalarBarSp()->SetTitle("Quality");
    w->scalarBarSp()->SetNumberOfLabels(5);
    w->scalarBarSp()->SetWidth(0.10);
    w->scalarBarSp()->SetHeight(0.45);
    w->scalarBarSp()->SetPosition(0.88, 0.25);
    w->scalarBarSp()->GetTitleTextProperty()->SetColor(1, 1, 1);
    w->scalarBarSp()->GetLabelTextProperty()->SetColor(1, 1, 1);
    w->rendererSp()->AddActor2D(w->scalarBarSp());

    w->comboLegendSp() = vtkSmartPointer<vtkLegendBoxActor>::New();
    w->comboLegendSp()->SetNumberOfEntries(0);
    w->comboLegendSp()->UseBackgroundOn();
    {
        const double* bg = w->rendererSp()->GetBackground();
        w->comboLegendSp()->SetBackgroundColor(bg[0], bg[1], bg[2]);
    }
    w->comboLegendSp()->SetBackgroundOpacity(0.70);
    w->comboLegendSp()->SetBorder(0);
    w->comboLegendSp()->SetPadding(3);
    w->comboLegendSp()->LockBorderOff();

    const QString cjkFontFile = pickChineseFontFile();
    if (!cjkFontFile.isEmpty())
    {
        static std::string sCjkFontFileUtf8;
        sCjkFontFileUtf8 = cjkFontFile.toUtf8().toStdString();
        w->comboLegendSp()->GetEntryTextProperty()->SetFontFamily(VTK_FONT_FILE);
        w->comboLegendSp()->GetEntryTextProperty()->SetFontFile(sCjkFontFileUtf8.c_str());
    }
    else
    {
        w->comboLegendSp()->GetEntryTextProperty()->SetFontFamilyAsString("Microsoft YaHei");
    }
    w->comboLegendSp()->GetEntryTextProperty()->SetColor(1, 1, 1);

    const double legX = 0.015;
    const double legY = 0.825;
    const double legW = 0.26;
    const double legH = 0.14;
    w->comboLegendSp()->GetEntryTextProperty()->SetFontSize(12);
    w->comboLegendSp()->SetPosition(legX, legY);
    w->comboLegendSp()->SetWidth(legW);
    w->comboLegendSp()->SetHeight(legH);
    w->rendererSp()->AddActor2D(w->comboLegendSp());
    w->comboLegendSp()->SetVisibility(false);

    w->comboLegendTitleSp() = vtkSmartPointer<vtkTextActor>::New();
    if (!cjkFontFile.isEmpty())
    {
        static std::string sCjkFontFileUtf8Title;
        sCjkFontFileUtf8Title = cjkFontFile.toUtf8().toStdString();
        w->comboLegendTitleSp()->GetTextProperty()->SetFontFamily(VTK_FONT_FILE);
        w->comboLegendTitleSp()->GetTextProperty()->SetFontFile(sCjkFontFileUtf8Title.c_str());
    }
    else
    {
        w->comboLegendTitleSp()->GetTextProperty()->SetFontFamilyAsString("Microsoft YaHei");
    }
    w->comboLegendTitleSp()->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
    w->comboLegendTitleSp()->GetTextProperty()->SetBold(true);
    w->comboLegendTitleSp()->GetTextProperty()->SetFontSize(18);
    static const std::string kLegendTitleUtf8 = u8"病态网格元素";
    w->comboLegendTitleSp()->SetInput(kLegendTitleUtf8.c_str());
    w->comboLegendTitleSp()->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    w->comboLegendTitleSp()->SetPosition(legX + 0.003, legY + legH + 0.01);
    w->rendererSp()->AddActor2D(w->comboLegendTitleSp());
    w->comboLegendTitleSp()->SetVisibility(false);
}

void SceneController::pickFromInteractor(mainwindow* w, const vtklasttry::SceneViewRequest& req)
{
    if (!w || !w->cellPickerSp() || !w->rendererSp() || !w->meshDataSp())
        return;

    vtkRenderWindowInteractor* iren = w->vtkWidget() ? w->vtkWidget()->GetInteractor() : nullptr;
    if (!iren)
        return;

    const vtklasttry::PickResult r = vtklasttry::PickController::pickFromInteractor(w->cellPickerSp(),
                                                                                   w->rendererSp(),
                                                                                   w->meshDataSp(),
                                                                                   w->qualityArraySp(),
                                                                                   iren);
    if (!r.picked)
        return;

    w->selectedCellIdRef() = r.cellId;
    SceneController::updateSelectedCellOverlay(w, static_cast<long long>(r.cellId));
    if (req.isolateSelectedBad)
        SceneController::applyIsolationMode(w, true, req);

    vtklasttry::PickController::logPick(w->uiTextLog(), w->statusBar(), r);
    if (w->renderWindowSp())
        w->renderWindowSp()->Render();
}

void SceneController::updateVisualization(mainwindow* w, const vtklasttry::SceneViewRequest& req)
{
    if (!w || !w->meshDataSp() || !w->qualityArraySp())
        return;

    const bool comboEnabled = req.comboEnabled;

    vtkNew<vtkDataSetSurfaceFilter> surface;
    surface->SetInputData(w->meshDataSp());
    surface->Update();

    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputConnection(surface->GetOutputPort());
    if (comboEnabled)
    {
        mapper->ScalarVisibilityOff();
        if (w->scalarBarSp()) w->scalarBarSp()->SetVisibility(false);
        w->setComboLegendChromeVisiblePublic(true);
    }
    else
    {
        mapper->SetScalarModeToUseCellData();
        mapper->SelectColorArray(w->qualityArraySp()->GetName());
        mapper->SetLookupTable(w->lutSp());
        mapper->ScalarVisibilityOn();

        double range[2] = { 0.0, 1.0 };
        w->qualityArraySp()->GetRange(range);
        mapper->SetScalarRange(range);
        if (w->lutSp())
            w->lutSp()->SetRange(range);
        if (w->scalarBarSp())
            w->scalarBarSp()->SetLookupTable(w->lutSp());
        if (w->scalarBarSp()) w->scalarBarSp()->SetVisibility(true);
        w->setComboLegendChromeVisiblePublic(false);
    }

    if (!w->meshActorSp())
        w->meshActorSp() = vtkSmartPointer<vtkActor>::New();
    w->meshActorSp()->SetMapper(mapper);
    w->meshActorSp()->GetProperty()->SetOpacity(1.0);
    w->meshActorSp()->GetProperty()->SetRepresentationToSurface();
    w->meshActorSp()->GetProperty()->EdgeVisibilityOn();
    w->meshActorSp()->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
    w->meshActorSp()->GetProperty()->SetLineWidth(1.0);
    if (comboEnabled)
        w->meshActorSp()->GetProperty()->SetColor(0.20, 0.75, 0.20);
}

void SceneController::updateBadCellsOverlay(mainwindow* w,
                                            const vtklasttry::OverlayRequest& req)
{
    if (!w || !w->meshDataSp() || !w->qualityArraySp())
        return;

    if (!req.showBad)
    {
        if (w->badActorSp())
            w->rendererSp()->RemoveActor(w->badActorSp());
        for (auto& a : w->comboBadActorsRef())
            if (a) w->rendererSp()->RemoveActor(a);
        w->comboBadActorsRef().clear();
        w->setComboLegendChromeVisiblePublic(false);
        vtklasttry::BadCellsPanel::clear(w->uiBadCellsList());
        return;
    }

    const bool comboEnabled = req.comboEnabled;
    const QString metricKey1 = req.metricKeySingle;
    const double thr1 = req.thresholdSingle;
    const bool isGE1 = req.isGESingle;

    const QString op = req.logicOp; // and/or
    const QVector<vtklasttry::RuleCondition> tableConds = req.tableConds;

    if (!comboEnabled)
    {
        vtklasttry::appendUiLog(w->uiTextLog(), QString("bad-cell rule: q %1 %2").arg(isGE1 ? ">=" : "<=").arg(thr1, 0, 'g', 8));
        // Ensure multi-metric overlays are cleared.
        for (auto& a : w->comboBadActorsRef())
            if (a) w->rendererSp()->RemoveActor(a);
        w->comboBadActorsRef().clear();
        w->setComboLegendChromeVisiblePublic(false);
    }
    else
    {
        vtklasttry::appendUiLog(w->uiTextLog(), QString("bad-cell combo: %1").arg(req.comboRuleText));
        // Hide the single-metric overlay actor.
        if (w->badActorSp())
            w->rendererSp()->RemoveActor(w->badActorSp());
    }

    {
        double r[2] = { 0.0, 0.0 };
        w->qualityArraySp()->GetRange(r);
        vtklasttry::appendUiLog(w->uiTextLog(), QString("quality range: [%1, %2]").arg(r[0], 0, 'g', 8).arg(r[1], 0, 'g', 8));
    }

    const vtkIdType nCells = w->meshDataSp()->GetNumberOfCells();
    w->lastBadCellIdsRef().clear();
    vtklasttry::BadCellEvalResult eval;
    if (!comboEnabled)
    {
        vtkDataArray* q1 = w->qualityCacheRef().contains(metricKey1) ? w->qualityCacheRef().value(metricKey1) : w->qualityArraySp();
        eval = vtklasttry::evaluateBadCellsSingle(w->meshDataSp(), q1, metricKey1, isGE1, thr1);
    }
    else
    {
        eval = vtklasttry::evaluateBadCellsCombo(w->meshDataSp(), w->qualityCacheRef(), tableConds, op);
    }

    w->lastBadCellIdsRef() = eval.badIdsUnion;
    vtklasttry::appendUiLog(w->uiTextLog(), QString("bad cells selected: %1 / %2").arg(w->lastBadCellIdsRef().size()).arg(nCells));
    w->lastTotalCellsForStatsRef() = static_cast<int>(nCells);
    vtklasttry::BadCellsPanel::updateBadStatsLabel(w->uiBadStatsLabel(), w->lastBadCellIdsRef(), w->lastTotalCellsForStatsRef());

    vtklasttry::BadCellsPanel::renderList(w->uiBadCellsList(), w->lastBadCellIdsRef(), w->qualityArraySp());

    // Step 1: render-only part.
    vtklasttry::SceneController::updateBadCellsOverlayRender(w, comboEnabled, tableConds, eval, req.metricUiItems);
}

void SceneController::updateBadCellsOverlayRender(mainwindow* w,
                                                  bool comboEnabled,
                                                  const QVector<vtklasttry::RuleCondition>& tableConds,
                                                  const vtklasttry::BadCellEvalResult& eval,
                                                  const QVector<QPair<QString, QString>>& metricUiItems)
{
    if (!w || !w->meshDataSp() || !w->rendererSp())
        return;

    // Build overlay geometry from extracted cells (union overlay).
    vtkNew<vtkIdList> badIdList;
    badIdList->SetNumberOfIds(static_cast<vtkIdType>(eval.badIdsUnion.size()));
    for (int k = 0; k < eval.badIdsUnion.size(); ++k)
        badIdList->SetId(k, static_cast<vtkIdType>(eval.badIdsUnion[k]));

    vtkNew<vtkExtractCells> extract;
    extract->SetInputData(w->meshDataSp());
    extract->SetCellList(badIdList);
    extract->Update();

    vtkNew<vtkGeometryFilter> geom;
    geom->SetInputConnection(extract->GetOutputPort());
    geom->Update();

    vtkNew<vtkDataSetMapper> badMapper;
    badMapper->SetInputConnection(geom->GetOutputPort());
    badMapper->ScalarVisibilityOff();

    if (!w->badActorSp())
        w->badActorSp() = vtkSmartPointer<vtkActor>::New();
    w->badActorSp()->SetMapper(badMapper);
    w->badActorSp()->GetProperty()->SetColor(1.0, 0.0, 0.0);
    w->badActorSp()->GetProperty()->SetOpacity(0.55);

    if (!w->rendererSp()->HasViewProp(w->badActorSp()))
        w->rendererSp()->AddActor(w->badActorSp());

    // Multi-metric overlays + “病态网格元素” table legend (pass rate =XX.XX % per row)
    if (comboEnabled)
    {
        for (auto& a : w->comboBadActorsRef())
            if (a) w->rendererSp()->RemoveActor(a);
        w->comboBadActorsRef().clear();
        w->comboLegendEntryUtf8Ref().clear();
        w->comboBadColorByCellRef().clear();

        struct ComboLegPrepared
        {
            vtklasttry::RuleCondition cond;
            QVector<vtkIdType> badIds;
            int supported = 0;
        };
        QVector<ComboLegPrepared> prepared;
        prepared.reserve(tableConds.size());

        // Reuse eval results (avoid rescanning all cells again).
        // Keep the legend row count consistent with the rule table.
        const int nRules = tableConds.size();
        for (int r = 0; r < nRules; ++r)
        {
            ComboLegPrepared row;
            row.cond = tableConds[r];
            if (r < eval.badIdsByRule.size())
                row.badIds = eval.badIdsByRule[r];
            row.supported = (r < eval.supportedByRule.size()) ? eval.supportedByRule[r] : 0;
            prepared.push_back(std::move(row));
        }

        if (w->comboLegendSp())
            w->comboLegendSp()->SetNumberOfEntries(prepared.size());
        // Keep QByteArray storage stable: avoid reallocation invalidating const char* passed to VTK.
        w->comboLegendEntryUtf8Ref().reserve(prepared.size());
        // Auto-fit legend box height for >=3 rows (prevents clipping/overlap).
        if (w->comboLegendSp())
        {
            const double h = std::min(0.40, 0.09 + 0.035 * static_cast<double>(prepared.size()));
            // Keep legend top anchored inside viewport; grow downward for more rows.
            const double top = 0.965;
            const double y = std::max(0.02, top - h);
            w->comboLegendSp()->SetHeight(h);
            w->comboLegendSp()->SetPosition(0.015, y);
            if (w->comboLegendTitleSp())
            {
                w->comboLegendTitleSp()->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
                w->comboLegendTitleSp()->SetPosition(0.018, std::min(0.985, top + 0.003));
            }
        }

        const std::vector<std::array<double, 3>>& colors = pathologicLegendColors();
        QVector<QString> legendLabels;
        legendLabels.reserve(prepared.size());
        for (int ir = 0; ir < prepared.size(); ++ir)
        {
            const ComboLegPrepared& row = prepared[ir];
            const auto& col = colors[static_cast<size_t>(ir) % colors.size()];
            // Remember per-cell overlay color for isolation mode.
            for (int k = 0; k < row.badIds.size(); ++k)
                w->comboBadColorByCellRef().insert(row.badIds[k], col);

            // Only build an overlay actor when there are bad cells for this rule.
            if (!row.badIds.isEmpty())
            {
                vtkNew<vtkIdList> idList;
                idList->SetNumberOfIds(row.badIds.size());
                for (int k = 0; k < row.badIds.size(); ++k)
                    idList->SetId(k, row.badIds[k]);

                vtkNew<vtkExtractCells> ex;
                ex->SetInputData(w->meshDataSp());
                ex->SetCellList(idList);
                ex->Update();

                vtkNew<vtkGeometryFilter> g;
                g->SetInputConnection(ex->GetOutputPort());
                g->Update();

                vtkNew<vtkDataSetMapper> mm;
                mm->SetInputConnection(g->GetOutputPort());
                mm->ScalarVisibilityOff();

                vtkSmartPointer<vtkActor> a = vtkSmartPointer<vtkActor>::New();
                a->SetMapper(mm);
                a->GetProperty()->SetColor(col[0], col[1], col[2]);
                a->GetProperty()->SetOpacity(0.70);
                a->GetProperty()->EdgeVisibilityOff();

                w->comboBadActorsRef().push_back(a);
                w->rendererSp()->AddActor(a);
            }

            const vtkIdType badN = static_cast<vtkIdType>(row.badIds.size());
            const double passPct = (row.supported > 0)
                ? (100.0 * static_cast<double>(row.supported - badN) / static_cast<double>(row.supported))
                : 100.0;
            const QString nameZh = vtklasttry::metricNameFromUiItems(row.cond.metricKey, metricUiItems);
            // Keep '=' at a fixed visual column for better scanability.
            const QString nameCol = nameZh.left(10).leftJustified(10, QLatin1Char(' '));
            const QString badCol = QString::number(static_cast<qlonglong>(badN)).rightJustified(6, QLatin1Char(' '));
            const QString pctVal = QString::number(passPct, 'f', 2).rightJustified(6, QLatin1Char(' '));
            const QString label = QStringLiteral("%1 %2   =%3 %").arg(nameCol, badCol, pctVal);
            legendLabels.push_back(label);
        }

        // Two-phase legend setup: avoid pointer invalidation and ensure all rows stay valid.
        w->comboLegendEntryUtf8Ref().clear();
        w->comboLegendEntryUtf8Ref().reserve(legendLabels.size());
        for (int i = 0; i < legendLabels.size(); ++i)
            w->comboLegendEntryUtf8Ref().push_back(legendLabels[i].toUtf8());

        if (w->comboLegendSp())
        {
            for (int ir = 0; ir < prepared.size(); ++ir)
            {
                const auto& col = colors[static_cast<size_t>(ir) % colors.size()];
                double rgb[3] = { col[0], col[1], col[2] };
                vtkNew<vtkPlaneSource> legSym;
                legSym->SetXResolution(1);
                legSym->SetYResolution(1);
                legSym->SetOrigin(0.0, 0.0, 0.0);
                legSym->SetPoint1(1.0, 0.0, 0.0);
                legSym->SetPoint2(0.0, 1.0, 0.0);
                legSym->Update();
                w->comboLegendSp()->SetEntry(ir, legSym->GetOutput(), w->comboLegendEntryUtf8Ref()[ir].constData(), rgb);
            }
        }

        w->setComboLegendChromeVisiblePublic(true);
    }
}

void SceneController::updateSelectedCellOverlay(mainwindow* w, long long cellId)
{
    if (!w)
        return;
    vtklasttry::SelectionController::updateSelectedCellOverlay(w->meshDataSp(),
                                                              w->rendererSp(),
                                                              w->uiTextLog(),
                                                              static_cast<vtkIdType>(cellId),
                                                              &w->selectedActorSp());
}
void SceneController::applyIsolationMode(mainwindow* w, bool enabled, const vtklasttry::SceneViewRequest& req)
{
    if (!w)
        return;
    const bool comboEnabled = req.comboEnabled;
    const bool showBad = req.showBad;
    vtklasttry::SelectionController::applyIsolationMode(w->meshDataSp(),
                                                       w->rendererSp(),
                                                       w->uiTextLog(),
                                                       enabled,
                                                       comboEnabled,
                                                       showBad,
                                                       w->selectedCellIdRef(),
                                                       w->meshActorSp(),
                                                       w->selectedActorSp(),
                                                       w->badActorSp(),
                                                       w->comboBadActorsRef(),
                                                       &w->isolatedActorSp(),
                                                       w->comboBadColorByCellRef(),
                                                       w->qualityArraySp(),
                                                       w->lutSp());
}
} // namespace vtklasttry

