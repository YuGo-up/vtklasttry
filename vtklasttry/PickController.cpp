#include "PickController.h"

#include "UiLog.h"

#include <QStatusBar>
#include <QTextEdit>

#include <vtkCellPicker.h>
#include <vtkCellTypes.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>

#include <cmath>

namespace vtklasttry
{
PickResult PickController::pickFromInteractor(vtkSmartPointer<vtkCellPicker> picker,
                                              vtkSmartPointer<vtkRenderer> renderer,
                                              vtkSmartPointer<vtkDataSet> mesh,
                                              vtkSmartPointer<vtkDataArray> qualityArray,
                                              vtkRenderWindowInteractor* interactor)
{
    PickResult r;
    if (!picker || !renderer || !mesh || !interactor)
        return r;

    int pos[2] = { 0, 0 };
    interactor->GetEventPosition(pos);
    const int x = pos[0];
    const int y = pos[1];

    if (!picker->Pick(x, y, 0.0, renderer))
        return r;

    const vtkIdType cellId = static_cast<vtkIdType>(picker->GetCellId());
    if (cellId < 0)
        return r;

    r.picked = true;
    r.cellId = cellId;

    const int ct = mesh->GetCellType(cellId);
    r.cellType = ct;
    const char* ctNameC = vtkCellTypes::GetClassNameFromTypeId(ct);
    r.cellTypeName = ctNameC ? QString::fromLatin1(ctNameC) : QStringLiteral("VTK_UNKNOWN");

    r.qText = QStringLiteral("N/A");
    if (qualityArray)
    {
        const double qv = qualityArray->GetTuple1(cellId);
        if (std::isfinite(qv))
            r.qText = QString::number(qv, 'g', 10);
    }
    return r;
}

void PickController::logPick(QTextEdit* logBox, QStatusBar* statusBar, const PickResult& r)
{
    if (!r.picked)
        return;
    vtklasttry::appendUiLog(logBox,
                            QString::fromWCharArray(L"\u62FE\u53D6\u5355\u5143\uFF1AcellId=%1, cellType=%2(%3), q=%4")
                                .arg(static_cast<qlonglong>(r.cellId))
                                .arg(r.cellTypeName)
                                .arg(r.cellType)
                                .arg(r.qText));
    if (statusBar)
        statusBar->showMessage(QString::fromWCharArray(L"\u62FE\u53D6\u5355\u5143\uFF1AcellId=%1").arg(static_cast<qlonglong>(r.cellId)));
}
} // namespace vtklasttry

