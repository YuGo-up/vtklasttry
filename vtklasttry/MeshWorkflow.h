#pragma once

#include "MeshSelectRequest.h"
#include "MeshState.h"

#include <QString>

class QLabel;
class QMainWindow;
class QStatusBar;
class QTextEdit;

class vtkActor;
class vtkDataArray;
class vtkDataSet;
class vtkGenericOpenGLRenderWindow;
class vtkLegendBoxActor;
class vtkRenderer;
class vtkScalarBarActor;
class vtkTextActor;
template <typename T> class vtkSmartPointer;

namespace vtklasttry
{
class MeshWorkflow
{
public:
    static void onSelectFile(QMainWindow* parent,
                             QTextEdit* logBox,
                             QStatusBar* statusBar,
                             const vtklasttry::MeshSelectRequest& req,
                             vtklasttry::MeshState& state);

    static bool loadMesh(QMainWindow* parent,
                         QTextEdit* logBox,
                         QStatusBar* statusBar,
                         vtkSmartPointer<vtkRenderer> renderer,
                         vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow,
                         const QString& path,
                         QString* outPipelineText,
                         vtkSmartPointer<vtkDataSet>* outMeshData,
                         QString* outMeshClassText,
                         QString* outCellTypeStatsTableMd,
                         vtkSmartPointer<vtkActor>* inOutMeshActor,
                         vtkSmartPointer<vtkScalarBarActor> scalarBar,
                         vtkSmartPointer<vtkLegendBoxActor> comboLegend,
                         vtkSmartPointer<vtkTextActor> comboLegendTitle);
};
} // namespace vtklasttry

