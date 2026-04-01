#include "MeshAnalysis.h"

#include <map>
#include <vector>
#include <algorithm>

#include <vtkCellTypes.h>
#include <vtkDataSet.h>

#include "QualityEngine.h"
#include "QualitySupport.h"

namespace
{
static QString cellTypeEnumName(int ct)
{
    switch (ct)
    {
    case 0: return QStringLiteral("VTK_EMPTY_CELL");
    case 1: return QStringLiteral("VTK_VERTEX");
    case 2: return QStringLiteral("VTK_POLY_VERTEX");
    case 3: return QStringLiteral("VTK_LINE");
    case 4: return QStringLiteral("VTK_POLY_LINE");
    case 5: return QStringLiteral("VTK_TRIANGLE");
    case 6: return QStringLiteral("VTK_TRIANGLE_STRIP");
    case 7: return QStringLiteral("VTK_POLYGON");
    case 8: return QStringLiteral("VTK_PIXEL");
    case 9: return QStringLiteral("VTK_QUAD");
    case 10: return QStringLiteral("VTK_TETRA");
    case 11: return QStringLiteral("VTK_VOXEL");
    case 12: return QStringLiteral("VTK_HEXAHEDRON");
    case 13: return QStringLiteral("VTK_WEDGE");
    case 14: return QStringLiteral("VTK_PYRAMID");
    case 15: return QStringLiteral("VTK_PENTAGONAL_PRISM");
    case 16: return QStringLiteral("VTK_HEXAGONAL_PRISM");
    case 21: return QStringLiteral("VTK_QUADRATIC_EDGE");
    case 22: return QStringLiteral("VTK_QUADRATIC_TRIANGLE");
    case 23: return QStringLiteral("VTK_QUADRATIC_QUAD");
    case 24: return QStringLiteral("VTK_QUADRATIC_TETRA");
    case 25: return QStringLiteral("VTK_QUADRATIC_HEXAHEDRON");
    case 26: return QStringLiteral("VTK_QUADRATIC_WEDGE");
    case 27: return QStringLiteral("VTK_QUADRATIC_PYRAMID");
    default:
        break;
    }
    const char* n = vtkCellTypes::GetClassNameFromTypeId(ct);
    return n ? QString("%1 (%2)").arg(QStringLiteral("VTK_UNKNOWN"), QString::fromLatin1(n)) : QStringLiteral("VTK_UNKNOWN");
}

static bool is2DCellType(int ct)
{
    switch (ct)
    {
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 22:
    case 23:
        return true;
    default:
        return false;
    }
}

static bool is3DCellType(int ct)
{
    switch (ct)
    {
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 24:
    case 25:
    case 26:
    case 27:
        return true;
    default:
        return false;
    }
}

static bool is1DCellType(int ct)
{
    switch (ct)
    {
    case 3:
    case 4:
    case 21:
        return true;
    default:
        return false;
    }
}

static bool is0DCellType(int ct) { return (ct == 1 || ct == 2); }
} // namespace

namespace vtklasttry
{
MeshAnalysisResult analyzeMesh(vtkDataSet* ds)
{
    MeshAnalysisResult out;
    if (!ds)
        return out;

    std::map<int, int> counts;
    bool has2D = false, has3D = false, has1D = false, has0D = false;

    const vtkIdType nCells = ds->GetNumberOfCells();
    for (vtkIdType i = 0; i < nCells; ++i)
    {
        const int ct = ds->GetCellType(i);
        counts[ct] += 1;
        if (is3DCellType(ct)) has3D = true;
        else if (is2DCellType(ct)) has2D = true;
        else if (is1DCellType(ct)) has1D = true;
        else if (is0DCellType(ct)) has0D = true;
    }

    if (has3D && has2D) out.meshClassText = QString::fromWCharArray(L"\u6DF7\u5408\u7F51\u683C\uFF082D+3D\uFF09");
    else if (has3D) out.meshClassText = QString::fromWCharArray(L"\u4F53\u7F51\u683C\uFF083D\uFF09");
    else if (has2D) out.meshClassText = QString::fromWCharArray(L"\u8868\u9762\u7F51\u683C\uFF082D\uFF09");
    else if (has1D) out.meshClassText = QString::fromWCharArray(L"\u7EBF\u7F51\u683C\uFF081D\uFF09");
    else if (has0D) out.meshClassText = QString::fromWCharArray(L"\u70B9\u96C6\uFF080D\uFF09");
    else out.meshClassText = QString::fromWCharArray(L"\u672A\u77E5\u7C7B\u578B");

    struct Row { int ct; int count; };
    std::vector<Row> rows;
    rows.reserve(static_cast<size_t>(counts.size()));
    for (const auto& kv : counts)
        rows.push_back(Row{ kv.first, kv.second });
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.count > b.count; });

    QString table;
    for (const Row& r : rows)
    {
        const double ratio = (nCells > 0) ? (100.0 * r.count / static_cast<double>(nCells)) : 0.0;
        table += QString("| %1 | %2 | %3 | %4 |\n")
                     .arg(r.ct)
                     .arg(cellTypeEnumName(r.ct))
                     .arg(r.count)
                     .arg(QString::number(ratio, 'f', 2));
    }
    if (table.isEmpty())
        table = QString("| - | - | - | - |\n");
    out.cellTypeStatsTableMd = table.trimmed();
    return out;
}

QString buildMetricSupportSummary(vtkDataSet* ds, const QString& metricKey)
{
    if (!ds)
        return QString();
    std::map<int, int> counts;
    const vtkIdType n = ds->GetNumberOfCells();
    for (vtkIdType i = 0; i < n; ++i)
    {
        const int ct = ds->GetCellType(i);
        if (!vtklasttry::isMetricSupportedCell(metricKey, ct))
            continue;
        counts[ct] += 1;
    }
    if (counts.empty())
        return QString();

    struct Row { int ct; int count; };
    std::vector<Row> rows;
    rows.reserve(static_cast<size_t>(counts.size()));
    for (const auto& kv : counts) rows.push_back(Row{kv.first, kv.second});
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.count > b.count; });

    QStringList parts;
    for (const Row& r : rows)
        parts << QString("%1:%2").arg(cellTypeEnumName(r.ct)).arg(r.count);
    return parts.join(QStringLiteral(", "));
}
} // namespace vtklasttry

