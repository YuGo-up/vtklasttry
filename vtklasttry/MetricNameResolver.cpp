#include "MetricNameResolver.h"

namespace vtklasttry
{
QString metricLegendNameZhByKey(const QString& metricKey)
{
    if (metricKey == "aspect") return QString::fromWCharArray(L"\u957F\u5BBD\u6BD4");
    if (metricKey == "skew") return QString::fromWCharArray(L"\u626D\u66F2\u5EA6");
    if (metricKey == "scaled_jacobian") return QString::fromWCharArray(L"\u7F29\u653E\u96C5\u53EF\u6BD4");
    if (metricKey == "jacobian") return QString::fromWCharArray(L"\u96C5\u53EF\u6BD4");
    if (metricKey == "jacobian_ratio") return QString::fromWCharArray(L"\u96C5\u53EF\u6BD4\u6BD4\u7387");
    if (metricKey == "min_angle") return QString::fromWCharArray(L"\u6700\u5C0F\u89D2");
    if (metricKey == "max_angle") return QString::fromWCharArray(L"\u6700\u5927\u89D2");
    if (metricKey == "min_edge_length") return QString::fromWCharArray(L"\u6700\u5C0F\u8FB9\u957F");
    if (metricKey == "non_ortho") return QString::fromWCharArray(L"\u975E\u6B63\u4EA4\u89D2");
    return QString();
}

QString metricNameFromUiItems(const QString& metricKey,
                              const QVector<QPair<QString, QString>>& metricItems)
{
    const QString byKey = metricLegendNameZhByKey(metricKey);
    if (!byKey.isEmpty())
        return byKey;

    for (int i = 0; i < metricItems.size(); ++i)
    {
        if (metricItems[i].first != metricKey)
            continue;

        const QString t = metricItems[i].second;
        const int l1 = t.indexOf(QChar(0xFF08)); // （
        const int r1 = t.indexOf(QChar(0xFF09)); // ）
        if (l1 >= 0 && r1 > l1)
        {
            const QString inside = t.mid(l1 + 1, r1 - l1 - 1).trimmed();
            if (!inside.isEmpty())
                return inside;
        }

        const int l2 = t.indexOf('('); // (
        const int r2 = t.indexOf(')'); // )
        if (l2 >= 0 && r2 > l2)
        {
            const QString inside = t.mid(l2 + 1, r2 - l2 - 1).trimmed();
            if (!inside.isEmpty())
                return inside;
        }

        if (!t.trimmed().isEmpty())
            return t;
        break;
    }

    return metricKey.isEmpty() ? QStringLiteral("?") : metricKey;
}
} // namespace vtklasttry
