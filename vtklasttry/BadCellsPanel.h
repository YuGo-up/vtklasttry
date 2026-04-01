#pragma once

#include <QVector>

class QLabel;
class QListWidget;
class vtkDataArray;
using vtkIdType = long long;

namespace vtklasttry
{
class BadCellsPanel
{
public:
    static void clear(QListWidget* list);
    static void updateBadStatsLabel(QLabel* label, const QVector<vtkIdType>& badIds, int totalCellsForStats);
    static void renderList(QListWidget* list, const QVector<vtkIdType>& badIds, vtkDataArray* qualityArray);
};
} // namespace vtklasttry

