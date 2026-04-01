#pragma once

#include <QString>

class QStatusBar;
class QTextEdit;

class vtkCellPicker;
class vtkDataArray;
class vtkDataSet;
class vtkRenderer;
class vtkRenderWindowInteractor;
template <typename T> class vtkSmartPointer;
using vtkIdType = long long;

namespace vtklasttry
{
struct PickResult
{
    bool picked = false;
    vtkIdType cellId = -1;
    int cellType = -1;
    QString cellTypeName;
    QString qText;
};

class PickController
{
public:
    static PickResult pickFromInteractor(vtkSmartPointer<vtkCellPicker> picker,
                                         vtkSmartPointer<vtkRenderer> renderer,
                                         vtkSmartPointer<vtkDataSet> mesh,
                                         vtkSmartPointer<vtkDataArray> qualityArray,
                                         vtkRenderWindowInteractor* interactor);

    static void logPick(QTextEdit* logBox, QStatusBar* statusBar, const PickResult& r);
};
} // namespace vtklasttry

