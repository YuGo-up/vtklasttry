#pragma once

#include <atomic>
#include <functional>
#include <QHash>
#include <QString>

#include "ComputeRequest.h"
#include "AppState.h"

class QObject;
class QProgressDialog;
class QPushButton;
class QStatusBar;
class QTextEdit;
template <typename T> class QFutureWatcher;

class vtkDataArray;
class vtkDataSet;
class vtkGenericOpenGLRenderWindow;
template <typename T> class vtkSmartPointer;

namespace vtklasttry
{
struct ComputeResult;

class ComputeWorkflow
{
public:
    static void onComputeAndShow(QObject* uiThreadObject,
                                 QObject* parentForDialogs,
                                 QTextEdit* logBox,
                                 QStatusBar* statusBar,
                                 QPushButton* computeBtn,
                                 QPushButton* importBtn,
                                 const vtklasttry::ComputeRequest& req,
                                 vtklasttry::AppState& state,
                                 const std::function<void()>& updateVisualization,
                                 const std::function<void()>& updateBadCellsOverlay,
                                 const std::function<void()>& flushVtkLogToUi);
};
} // namespace vtklasttry

