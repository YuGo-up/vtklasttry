#pragma once

#include <vtkSmartPointer.h>

class QTextEdit;
class vtkStringOutputWindow;

namespace vtklasttry
{
// Flush captured VTK output to a QTextEdit, then reset capture window.
// If there is no new output, appends a small "(VTK) 无新输出" message.
void flushVtkLogToUi(QTextEdit* box, vtkSmartPointer<vtkStringOutputWindow>& inOutCapture);
} // namespace vtklasttry

