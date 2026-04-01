#pragma once

#include <QPair>
#include <QString>
#include <QVector>

namespace vtklasttry
{
QString metricLegendNameZhByKey(const QString& metricKey);
QString metricNameFromUiItems(const QString& metricKey,
                              const QVector<QPair<QString, QString>>& metricItems);
} // namespace vtklasttry
