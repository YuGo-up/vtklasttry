#include "BadCellsPanel.h"

#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVariant>

#include <vtkDataArray.h>

namespace vtklasttry
{
void BadCellsPanel::clear(QListWidget* list)
{
    if (!list)
        return;
    list->clear();
}

void BadCellsPanel::updateBadStatsLabel(QLabel* label, const QVector<vtkIdType>& badIds, int totalCellsForStats)
{
    if (!label)
        return;
    const int bad = static_cast<int>(badIds.size());
    const int total = (totalCellsForStats > 0) ? totalCellsForStats : 0;
    const double ratio = (total > 0) ? (100.0 * bad / total) : 0.0;
    label->setText(QString::fromWCharArray(L"\u574F\u5355\u5143\u7EDF\u8BA1\uFF1A%1 / %2\uFF08%3%\uFF09")
                       .arg(bad)
                       .arg(total)
                       .arg(ratio, 0, 'f', 2));
}

void BadCellsPanel::renderList(QListWidget* list, const QVector<vtkIdType>& badIds, vtkDataArray* qualityArray)
{
    if (!list)
        return;

    list->clear();
    if (badIds.isEmpty())
    {
        list->addItem(new QListWidgetItem(QString::fromWCharArray(
            L"\u574F\u5355\u5143\u6570\u91CF\uFF1A0\uFF08\u8BF7\u5C1D\u8BD5\u8C03\u5927/\u8C03\u5C0F\u9608\u503C\u6216\u5207\u6362\u6307\u6807\uFF09")));
        return;
    }

    list->addItem(new QListWidgetItem(QString::fromWCharArray(L"\u574F\u5355\u5143\u6570\u91CF\uFF1A%1")
                                          .arg(static_cast<qlonglong>(badIds.size()))));

    const int maxShow = std::min(badIds.size(), 500);
    for (int k = 0; k < maxShow; ++k)
    {
        const vtkIdType cellId = badIds[k];
        const double qv = qualityArray ? qualityArray->GetTuple1(cellId) : 0.0;
        QListWidgetItem* item = new QListWidgetItem(QString("cellId=%1, q=%2")
                                                        .arg(static_cast<qlonglong>(cellId))
                                                        .arg(qv, 0, 'g', 6));
        item->setData(Qt::UserRole, QVariant::fromValue<qlonglong>(static_cast<qlonglong>(cellId)));
        list->addItem(item);
    }
    if (badIds.size() > maxShow)
        list->addItem(new QListWidgetItem(QString("... (%1 total)").arg(static_cast<qlonglong>(badIds.size()))));
}
} // namespace vtklasttry

