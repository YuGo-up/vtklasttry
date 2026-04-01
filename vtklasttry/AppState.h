#pragma once

#include <QHash>
#include <QString>
#include <atomic>

#include <vtkSmartPointer.h>

class QProgressDialog;
template <typename T> class QFutureWatcher;

class vtkDataArray;
class vtkDataSet;
class vtkGenericOpenGLRenderWindow;

namespace vtklasttry
{
struct ComputeResult;

struct AppState
{
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;
    vtkSmartPointer<vtkDataSet>* meshData = nullptr;
    QHash<QString, vtkSmartPointer<vtkDataArray>>* qualityCache = nullptr;
    vtkSmartPointer<vtkDataArray>* qualityArray = nullptr;
    QString* lastQualityMetricKey = nullptr;

    QFutureWatcher<vtklasttry::ComputeResult>** watcher = nullptr;
    QProgressDialog** progress = nullptr;
    std::atomic<bool>* cancelFlag = nullptr;
    bool* lastComputeCanceled = nullptr;
};
} // namespace vtklasttry

