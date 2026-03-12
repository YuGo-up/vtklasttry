#pragma once

#include <QString>

#include <vtkSmartPointer.h>

class vtkPolyData;

// 简单的网格数据模型，当前支持从 STL 读取三角网格
class MeshModel
{
public:
    MeshModel() = default;

    bool loadFromStl(const QString& filePath, QString& errorMessage);

    vtkSmartPointer<vtkPolyData> polyData() const { return m_polyData; }
    bool hasMesh() const;

private:
    vtkSmartPointer<vtkPolyData> m_polyData;
};

