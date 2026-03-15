#pragma once

#include <cstdint>
#include <string>

#include <vtkSmartPointer.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
class vtkLookupTable;

class VTKPipeline
{
public:
    VTKPipeline();

    vtkGenericOpenGLRenderWindow* renderWindow() const;

    void setMesh(vtkSmartPointer<vtkPolyData> polyData);

    void applyQualityScalar(const std::string& arrayName,
                            double minVal,
                            double maxVal);

    void updateBadCellsHighlight();

    /** Returns cell id or -1 if none picked. */
    int64_t pickCell(int displayX, int displayY);

    void resetCamera();

private:
    void ensureActor();
    void setupDefaultBackground();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer>                  m_renderer;
    vtkSmartPointer<vtkPolyDataMapper>            m_mapper;
    vtkSmartPointer<vtkActor>                     m_actor;
    vtkSmartPointer<vtkActor>                     m_badCellActor;
    vtkSmartPointer<vtkLookupTable>               m_lut;

    vtkSmartPointer<vtkPolyData>                  m_polyData;
};

