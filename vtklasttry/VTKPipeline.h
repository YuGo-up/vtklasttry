#pragma once

#include <string>

#include <vtkSmartPointer.h>

class vtkGenericOpenGLRenderWindow;
class vtkRenderer;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkActor;
class vtkLookupTable;

// 封装 VTK 渲染管线，负责把网格与质量标量以颜色映射方式显示出来
class VTKPipeline
{
public:
    VTKPipeline();

    vtkGenericOpenGLRenderWindow* renderWindow() const;

    void setMesh(vtkSmartPointer<vtkPolyData> polyData);

    // 根据给定的标量数组名称和范围应用颜色映射
    void applyQualityScalar(const std::string& arrayName,
                            double minVal,
                            double maxVal);

    void resetCamera();

private:
    void ensureActor();
    void setupDefaultBackground();

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderer>                  m_renderer;
    vtkSmartPointer<vtkPolyDataMapper>            m_mapper;
    vtkSmartPointer<vtkActor>                     m_actor;
    vtkSmartPointer<vtkLookupTable>               m_lut;

    vtkSmartPointer<vtkPolyData>                  m_polyData;
};

