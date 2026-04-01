#include "SelectionController.h"

#include "UiLog.h"

#include <QTextEdit>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractCells.h>
#include <vtkGeometryFilter.h>
#include <vtkIdList.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

namespace vtklasttry
{
void SelectionController::updateSelectedCellOverlay(vtkSmartPointer<vtkDataSet> mesh,
                                                    vtkSmartPointer<vtkRenderer> renderer,
                                                    QTextEdit* logBox,
                                                    vtkIdType cellId,
                                                    vtkSmartPointer<vtkActor>* inOutSelectedActor)
{
    if (!mesh || !renderer || !inOutSelectedActor)
        return;
    if (cellId < 0 || cellId >= mesh->GetNumberOfCells())
        return;

    vtkNew<vtkIdList> one;
    one->SetNumberOfIds(1);
    one->SetId(0, static_cast<vtkIdType>(cellId));

    vtkNew<vtkExtractCells> extract;
    extract->SetInputData(mesh);
    extract->SetCellList(one);
    extract->Update();

    vtkNew<vtkOutlineFilter> outline;
    outline->SetInputConnection(extract->GetOutputPort());
    outline->Update();

    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputConnection(outline->GetOutputPort());
    mapper->ScalarVisibilityOff();

    if (!(*inOutSelectedActor))
        *inOutSelectedActor = vtkSmartPointer<vtkActor>::New();
    (*inOutSelectedActor)->SetMapper(mapper);
    (*inOutSelectedActor)->GetProperty()->SetColor(1.0, 1.0, 0.0);
    (*inOutSelectedActor)->GetProperty()->SetLineWidth(3.0);

    if (!renderer->HasViewProp(*inOutSelectedActor))
        renderer->AddActor(*inOutSelectedActor);

    double b[6];
    mesh->GetCellBounds(static_cast<vtkIdType>(cellId), b);
    const double cx = 0.5 * (b[0] + b[1]);
    const double cy = 0.5 * (b[2] + b[3]);
    const double cz = 0.5 * (b[4] + b[5]);
    renderer->GetActiveCamera()->SetFocalPoint(cx, cy, cz);
    renderer->ResetCameraClippingRange();

    if (logBox)
    {
        vtklasttry::appendUiLog(logBox, QString("focus cellId=%1, center=(%2,%3,%4)")
                                            .arg(static_cast<qlonglong>(cellId))
                                            .arg(cx, 0, 'g', 6)
                                            .arg(cy, 0, 'g', 6)
                                            .arg(cz, 0, 'g', 6));
    }
}

void SelectionController::applyIsolationMode(vtkSmartPointer<vtkDataSet> mesh,
                                             vtkSmartPointer<vtkRenderer> renderer,
                                             QTextEdit* logBox,
                                             bool enabled,
                                             bool comboEnabled,
                                             bool showBad,
                                             vtkIdType selectedCellId,
                                             vtkSmartPointer<vtkActor> meshActor,
                                             vtkSmartPointer<vtkActor> selectedActor,
                                             vtkSmartPointer<vtkActor> badActor,
                                             const QVector<vtkSmartPointer<vtkActor>>& comboBadActors,
                                             vtkSmartPointer<vtkActor>* inOutIsolatedActor,
                                             const QHash<vtkIdType, std::array<double, 3>>& comboBadColorByCell,
                                             vtkSmartPointer<vtkDataArray> activeQualityArray,
                                             vtkSmartPointer<vtkLookupTable> lut)
{
    if (!mesh || !renderer || !inOutIsolatedActor)
        return;

    if (!enabled)
    {
        if (meshActor) meshActor->SetVisibility(true);
        if (selectedActor) selectedActor->SetVisibility(true);
        if (badActor) badActor->SetVisibility(showBad && !comboEnabled);
        for (auto& a : comboBadActors)
            if (a) a->SetVisibility(showBad && comboEnabled);
        if (*inOutIsolatedActor) (*inOutIsolatedActor)->SetVisibility(false);
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u5DF2\u9000\u51FA\u5355\u5143\u5355\u72EC\u663E\u793A\u6A21\u5F0F\u3002"));
        return;
    }

    if (meshActor) meshActor->SetVisibility(false);
    if (badActor) badActor->SetVisibility(false);
    for (auto& a : comboBadActors)
        if (a) a->SetVisibility(false);
    if (selectedActor) selectedActor->SetVisibility(false);

    if (selectedCellId < 0 || selectedCellId >= mesh->GetNumberOfCells())
    {
        vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u8BF7\u5148\u9009\u4E2D\u4E00\u4E2A\u574F\u5355\u5143\u3002"));
        return;
    }

    vtkNew<vtkIdList> one;
    one->SetNumberOfIds(1);
    one->SetId(0, selectedCellId);

    vtkNew<vtkExtractCells> extract;
    extract->SetInputData(mesh);
    extract->SetCellList(one);
    extract->Update();

    vtkNew<vtkGeometryFilter> geom;
    geom->SetInputConnection(extract->GetOutputPort());
    geom->Update();

    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputConnection(geom->GetOutputPort());
    if (comboEnabled)
    {
        // Combo mode renders base mesh with solid color + per-rule overlay colors.
        // Keep isolation consistent with that appearance (no scalar coloring).
        mapper->ScalarVisibilityOff();
    }
    else if (activeQualityArray)
    {
        mapper->SetScalarModeToUseCellData();
        mapper->SelectColorArray(activeQualityArray->GetName());
        mapper->SetLookupTable(lut);
        mapper->ScalarVisibilityOn();
        double range[2] = { 0.0, 1.0 };
        activeQualityArray->GetRange(range);
        if (lut)
            lut->SetRange(range);
        mapper->SetScalarRange(range);
    }
    else
    {
        mapper->ScalarVisibilityOff();
    }

    if (!(*inOutIsolatedActor))
        *inOutIsolatedActor = vtkSmartPointer<vtkActor>::New();
    (*inOutIsolatedActor)->SetMapper(mapper);
    (*inOutIsolatedActor)->GetProperty()->SetLineWidth(2.0);

    if (comboEnabled && comboBadColorByCell.contains(selectedCellId))
    {
        const auto col = comboBadColorByCell.value(selectedCellId);
        (*inOutIsolatedActor)->GetProperty()->SetColor(col[0], col[1], col[2]);
    }
    else if (comboEnabled)
    {
        // Not a "bad" cell in combo map: keep the base mesh solid color (typically green in combo mode).
        if (meshActor && meshActor->GetProperty())
        {
            double c[3] = { 0.20, 0.75, 0.20 };
            meshActor->GetProperty()->GetColor(c);
            (*inOutIsolatedActor)->GetProperty()->SetColor(c[0], c[1], c[2]);
        }
        else
        {
            (*inOutIsolatedActor)->GetProperty()->SetColor(0.20, 0.75, 0.20);
        }
    }
    else
    {
        if (!activeQualityArray)
        {
            // No scalar coloring available (e.g. right after import). Keep consistent with base mesh appearance.
            if (meshActor && meshActor->GetProperty())
            {
                double c[3] = { 0.85, 0.85, 0.85 };
                meshActor->GetProperty()->GetColor(c);
                (*inOutIsolatedActor)->GetProperty()->SetColor(c[0], c[1], c[2]);
            }
            else
            {
                (*inOutIsolatedActor)->GetProperty()->SetColor(0.85, 0.85, 0.85);
            }
        }
    }
    (*inOutIsolatedActor)->GetProperty()->SetOpacity(1.0);
    (*inOutIsolatedActor)->GetProperty()->EdgeVisibilityOn();
    (*inOutIsolatedActor)->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);

    if (!renderer->HasViewProp(*inOutIsolatedActor))
        renderer->AddActor(*inOutIsolatedActor);
    (*inOutIsolatedActor)->SetVisibility(true);

    vtklasttry::appendUiLog(logBox, QString::fromWCharArray(L"\u5355\u5143\u5355\u72EC\u663E\u793A\uFF1AcellId=%1").arg(static_cast<qlonglong>(selectedCellId)));
}
} // namespace vtklasttry

