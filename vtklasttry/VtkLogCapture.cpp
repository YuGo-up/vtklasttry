#include "VtkLogCapture.h"

#include <QTextEdit>

#include <vtkOutputWindow.h>
#include <vtkStringOutputWindow.h>

#include "UiLog.h"

namespace vtklasttry
{
void flushVtkLogToUi(QTextEdit* box, vtkSmartPointer<vtkStringOutputWindow>& inOutCapture)
{
    if (!box || !inOutCapture)
        return;
    const std::string s = inOutCapture->GetOutput();
    if (s.empty())
    {
        // Always show something so users know the log capture is active.
        vtklasttry::appendUiLog(box, QString::fromWCharArray(L"(VTK) \u65E0\u65B0\u8F93\u51FA"));
        return;
    }
    box->append(QString::fromUtf8(s.c_str()));
    // vtkStringOutputWindow (VTK 8.2) has no ClearOutput(); recreate instance to "clear".
    inOutCapture = vtkSmartPointer<vtkStringOutputWindow>::New();
    vtkOutputWindow::SetInstance(inOutCapture);
}
} // namespace vtklasttry

