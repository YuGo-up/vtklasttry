#pragma once

#include <array>

#include <QHash>
#include <QVector>

class QTextEdit;

class vtkActor;
class vtkDataArray;
class vtkDataSet;
class vtkGenericOpenGLRenderWindow;
class vtkLookupTable;
class vtkRenderer;
template <typename T> class vtkSmartPointer;
using vtkIdType = long long;

namespace vtklasttry
{
class SelectionController
{
public:
    static void updateSelectedCellOverlay(vtkSmartPointer<vtkDataSet> mesh,
                                         vtkSmartPointer<vtkRenderer> renderer,
                                         QTextEdit* logBox,
                                         vtkIdType cellId,
                                         vtkSmartPointer<vtkActor>* inOutSelectedActor);

    static void applyIsolationMode(vtkSmartPointer<vtkDataSet> mesh,
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
                                  vtkSmartPointer<vtkLookupTable> lut);
};
} // namespace vtklasttry

