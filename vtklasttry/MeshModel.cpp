#include "MeshModel.h"

#include <QFileInfo>

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkSTLReader.h>

bool MeshModel::loadFromStl(const QString& filePath, QString& errorMessage)
{
    errorMessage.clear();

    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile())
    {
        errorMessage = QObject::tr("File does not exist: %1").arg(filePath);
        m_polyData = nullptr;
        return false;
    }

    vtkSmartPointer<vtkSTLReader> reader =
        vtkSmartPointer<vtkSTLReader>::New();
    reader->SetFileName(filePath.toLocal8Bit().constData());
    reader->Update();

    vtkPolyData* output = reader->GetOutput();
    if (!output || output->GetNumberOfPoints() == 0)
    {
        errorMessage = QObject::tr("STL mesh is empty or failed to read.");
        m_polyData = nullptr;
        return false;
    }

    m_polyData = vtkSmartPointer<vtkPolyData>::New();
    m_polyData->DeepCopy(output);
    return true;
}

bool MeshModel::hasMesh() const
{
    return m_polyData != nullptr && m_polyData->GetNumberOfPoints() > 0;
}

