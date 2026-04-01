#pragma once

#include <functional>

#include <QString>

#include <vtkSmartPointer.h>

class vtkDataSet;

namespace vtklasttry
{
struct MeshLoadResult
{
    bool ok = false;
    QString sourcePath;       // original path user selected
    QString readablePath;     // possibly ASCII temp path
    QString pipeline;         // human-readable pipeline description
    QString message;          // optional info/warn
    vtkSmartPointer<vtkDataSet> data;
};

using LogFn = std::function<void(const QString&)>;

// Load mesh from path, including:
// - non-ASCII Windows path workaround (copy to temp ASCII)
// - legacy VTK 5.1 OFFSETS/CONNECTIVITY parse -> cached .vtu reuse
// - STL/VTP/VTU readers
// - Abaqus INP minimal parser (CPS4)
MeshLoadResult loadMesh(const QString& path, const LogFn& log);
} // namespace vtklasttry

