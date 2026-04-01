#include "ExportEngine.h"

#include <QDateTime>
#include <QSaveFile>
#include <QTextStream>

#include <vtkCellTypes.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkPNGWriter.h>
#include <vtkWindowToImageFilter.h>

namespace
{
static QString cellTypeName(int ct)
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

static QString csvQuote(QString s)
{
    s.replace(QChar('\"'), QChar('\''));
    return QString("\"%1\"").arg(s);
}
} // namespace

namespace vtklasttry
{
bool exportBadCellsCsv(const QString& path, const ExportCsvInput& in, QString* outError)
{
    if (!in.mesh || !in.displayQualityArray)
    {
        if (outError) *outError = QString::fromWCharArray(L"\u7F51\u683C/\u8D28\u91CF\u6570\u7EC4\u4E3A\u7A7A\uFF0C\u65E0\u6CD5\u5BFC\u51FA\u3002");
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (outError) *outError = QString::fromWCharArray(L"\u65E0\u6CD5\u5199\u5165\u6587\u4EF6\u3002");
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    out << "source_path," << csvQuote(in.sourcePath) << "\n";
    out << "display_metric_key," << in.displayMetricKey << "\n";
    out << "display_metric_name," << csvQuote(in.displayMetricName) << "\n";

    if (!in.comboEnabled)
    {
        out << "rule_key," << in.ruleKey << "\n";
        out << "rule_text," << csvQuote(in.ruleText) << "\n";
        out << "threshold," << in.threshold << "\n";
    }
    else
    {
        out << "combo_enabled,1\n";
        out << "combo_logic," << in.comboLogic << "\n";
        out << "combo_rule_text," << csvQuote(in.comboRuleText) << "\n";
        out << "rule_count," << in.comboRules.size() << "\n";
        for (int r = 0; r < in.comboRules.size(); ++r)
        {
            const RuleCondition& c = in.comboRules[r];
            out << "rule_" << r << "_metric_key," << c.metricKey << "\n";
            out << "rule_" << r << "_rule_key," << (c.isGE ? "ge" : "le") << "\n";
            out << "rule_" << r << "_threshold," << c.threshold << "\n";
            if (!c.templateKey.isEmpty())
                out << "rule_" << r << "_template_key," << c.templateKey << "\n";
        }
    }

    out << "mesh_cells," << in.mesh->GetNumberOfCells() << "\n";
    out << "bad_cells," << in.badCellIds.size() << "\n";
    out << "\n";

    QString header = QStringLiteral("cell_id,cell_type,cell_type_name,quality");
    if (in.comboEnabled)
    {
        for (int j = 0; j < in.comboRules.size(); ++j)
            header += QStringLiteral(",q%1").arg(j + 1);
    }
    out << header << "\n";

    for (vtkIdType cellId : in.badCellIds)
    {
        const double qv = in.displayQualityArray->GetTuple1(cellId);
        const int ct = in.mesh ? in.mesh->GetCellType(cellId) : -1;
        out << static_cast<qlonglong>(cellId) << "," << ct << "," << cellTypeName(ct) << "," << qv;

        if (in.comboEnabled)
        {
            for (const RuleCondition& c : in.comboRules)
            {
                vtkDataArray* qa = in.qualityCache.value(c.metricKey);
                const double qj = qa ? qa->GetTuple1(cellId) : std::numeric_limits<double>::quiet_NaN();
                out << "," << qj;
            }
        }

        out << "\n";
    }

    if (!file.commit())
    {
        if (outError) *outError = QString::fromWCharArray(L"\u6587\u4EF6\u4FDD\u5B58\u5931\u8D25\u3002");
        return false;
    }
    return true;
}

bool exportScreenshotPng(vtkSmartPointer<vtkGenericOpenGLRenderWindow> rw, const QString& path, QString* outError)
{
    if (!rw)
    {
        if (outError) *outError = QString::fromWCharArray(L"\u7A97\u53E3\u4E3A\u7A7A\uFF0C\u65E0\u6CD5\u622A\u56FE\u3002");
        return false;
    }

    rw->Render();

    vtkNew<vtkWindowToImageFilter> w2i;
    w2i->SetInput(rw);
    w2i->SetScale(1);
    w2i->SetInputBufferTypeToRGBA();
    w2i->ReadFrontBufferOff();
    w2i->Update();

    vtkNew<vtkPNGWriter> writer;
    writer->SetFileName(path.toLocal8Bit().constData());
    writer->SetInputConnection(w2i->GetOutputPort());
    writer->Write();
    return true;
}

bool writeUtf8TextFile(const QString& path, const QString& content, QString* outError)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (outError) *outError = QString::fromWCharArray(L"\u65E0\u6CD5\u5199\u5165\u6587\u4EF6\u3002");
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << content;

    if (!file.commit())
    {
        if (outError) *outError = QString::fromWCharArray(L"\u6587\u4EF6\u4FDD\u5B58\u5931\u8D25\u3002");
        return false;
    }
    return true;
}

static QString qualityExplainZh(const QString& key)
{
    if (key == "aspect")
        return QString::fromWCharArray(L"\u7406\u60F3\u503C\u63A5\u8FD1 1\uFF0C\u503C\u8D8A\u5927\u901A\u5E38\u8868\u793A\u5355\u5143\u8D8A\u7626\u957F/\u8D8A\u4E0D\u89C4\u5219\uFF0C\u5BF9\u6570\u503C\u7A33\u5B9A\u6027\u4E0D\u5229\uFF08\u672C\u5DE5\u5177\u91C7\u7528 VTK/vtkMeshQuality \u7684\u76F8\u5E94\u5EA6\u91CF\uFF09\u3002");
    if (key == "skew")
        return QString::fromWCharArray(L"\u901A\u5E38\u503C\u8D8A\u5927\u8868\u793A\u5355\u5143\u626D\u66F2/\u7578\u53D8\u8D8A\u4E25\u91CD\uFF0C\u503C\u8D8A\u5C0F\u8D8A\u597D\uFF08\u672C\u5DE5\u5177\u91C7\u7528 VTK/vtkMeshQuality \u7684\u5EA6\u91CF\uFF0C\u4E0D\u4E00\u5B9A\u7B49\u540C\u4E8E Fluent/OpenFOAM \u7684 skewness \u5B9A\u4E49\uFF09\u3002");
    if (key == "scaled_jacobian")
        return QString::fromWCharArray(L"\u7406\u60F3\u503C\u63A5\u8FD1 1\uFF0C\u63A5\u8FD1 0 \u8868\u793A\u5355\u5143\u9000\u5316\uFF0C\u5C0F\u4E8E 0 \u901A\u5E38\u610F\u5473\u7740\u5355\u5143\u53D1\u751F\u7FFB\u8F6C\uFF08\u4E0D\u5408\u6CD5\uFF09\uFF0C\u56E0\u6B64\u5E38\u7528\u89C4\u5219\u662F\u201Cq \u2264 \u9608\u503C\u201D \u5224\u4E3A\u574F\u5355\u5143\u3002");
    if (key == "non_ortho")
        return QString::fromWCharArray(L"\u5355\u4F4D\u4E3A\u201C\u5EA6\u201D\uFF0C\u4EE5\uFF08\u9762\u6CD5\u5411\uFF09\u4E0E\uFF08\u76F8\u90BB\u5355\u5143\u4E2D\u5FC3\u8FDE\u7EBF\uFF09\u7684\u5939\u89D2\u8868\u793A\u975E\u6B63\u4EA4\u7A0B\u5EA6\uFF1A0 \u6700\u597D\uFF0C\u89D2\u5EA6\u8D8A\u5927\u901A\u5E38\u8D8A\u5DEE\uFF08\u672C\u5DE5\u5177\u5BF9\u6BCF\u4E2A\u4F53\u5355\u5143\u53D6\u5404\u5185\u90E8\u9762\u7684\u6700\u5927\u503C\uFF0C\u8FB9\u754C\u9762\u4F1A\u8DF3\u8FC7\uFF09\u3002");
    if (key == "jacobian")
        return QString::fromWCharArray(L"VTK/Verdict \u7684 Jacobian\uFF1B\u4E09\u89D2\u5F62\u5355\u5143 VTK 8.2 \u65E0\u72EC\u7ACB Jacobian \u52A9\u624B\uFF0C\u672C\u5DE5\u5177\u7528 Scaled Jacobian \u4F5C\u4E3A\u66FF\u4EE3\u3002\u8FD1 0 \u6216\u8D1F\u503C\u5E38\u8868\u793A\u9000\u5316/\u7FFB\u8F6C\u98CE\u9669\uFF0C\u5E38\u7528\u89C4\u5219\u201Cq \u2264 \u9608\u503C\u201D\u3002");
    if (key == "jacobian_ratio")
        return QString::fromWCharArray(L"\u672C\u9879\u91C7\u7528 Verdict \u7684 Edge ratio\uFF08\u6700\u957F\u8FB9/\u6700\u77ED\u8FB9\uFF09\u4F5C\u4E3A\u51E0\u4F55\u6BD4\u7387\u6307\u6807\uFF0C\u503C \u2265 1\uFF1B\u8D8A\u5927\u901A\u5E38\u8868\u793A\u5F62\u72B6\u8D8A\u201C\u7626\u957F\u201D\uFF0C\u5E38\u7528\u89C4\u5219\u201Cq \u2265 \u9608\u503C\u201D\u3002");
    if (key == "min_angle")
        return QString::fromWCharArray(L"\u6700\u5C0F\u5185\u89D2\uFF08\u5EA6\uFF09\uFF1B\u8D8A\u5927\u901A\u5E38\u8D8A\u597D\uFF08\u66F4\u63A5\u8FD1\u6B63\u89D2\uFF09\uFF0C\u5E38\u7528\u89C4\u5219\u201Cq \u2264 \u9608\u503C\u201D \u5224\u4E3A\u8FC7\u9519\u3002");
    if (key == "max_angle")
        return QString::fromWCharArray(L"\u6700\u5927\u5185\u89D2\uFF08\u5EA6\uFF09\uFF1B\u8D8A\u5C0F\u901A\u5E38\u8D8A\u597D\uFF0C\u8FC7\u5927\u5E38\u8868\u793A\u5F62\u72B6\u8FC7\u201C\u6241\u201D\u6216\u9000\u5316\u98CE\u9669\uFF0C\u5E38\u7528\u89C4\u5219\u201Cq \u2265 \u9608\u503C\u201D\u3002");
    if (key == "min_edge_length")
        return QString::fromWCharArray(L"\u5355\u5143\u5185\u6240\u6709\u8FB9\u4E2D\u7684\u6700\u5C0F\u8FB9\u957F\uFF08\u6A21\u578B\u7A7A\u95F4\u5750\u6807\u5355\u4F4D\uFF09\uFF1B\u7528\u4E8E\u8FC7\u5BC6/\u8FC7\u758F\u4E0E\u5C3A\u5EA6\u53D8\u5316\u7684\u8F85\u52A9\u5224\u8BFB\uFF0C\u5E38\u7528\u201Cq \u2264 \u9608\u503C\u201D \u6807\u8BB0\u8FC7\u5C0F\u8FB9\uFF08\u9700\u7ED3\u5408\u7F51\u683C\u5C3A\u5EA6\u8BBE\u7F6E\u9608\u503C\uFF09\u3002");
    return QString::fromWCharArray(L"\u8BE5\u503C\u7684\u610F\u4E49\u7531\u5F53\u524D\u6307\u6807\u5B9A\u4E49\u51B3\u5B9A\uFF0C\u53C2\u89C1\u62A5\u544A\u4E2D\u7684 metric_key/\u6307\u6807\u540D\u79F0\u3002");
}

bool buildReportMarkdown(const ExportReportInput& in, QString* outMarkdown, QString* outError)
{
    if (!outMarkdown)
    {
        if (outError) *outError = QStringLiteral("outMarkdown is null");
        return false;
    }
    if (!in.mesh || !in.qualityArray)
    {
        if (outError) *outError = QString::fromWCharArray(L"\u7F51\u683C/\u8D28\u91CF\u6570\u7EC4\u4E3A\u7A7A\uFF0C\u65E0\u6CD5\u751F\u6210\u62A5\u544A\u3002");
        return false;
    }
    QString tpl = in.templateMarkdown;
    auto repl = [&](const QString& key, const QString& value) {
        tpl.replace(QString("{{%1}}").arg(key), value);
    };

    repl("TIME", QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    repl("SOURCE_PATH", in.sourcePath);
    repl("POINTS", QString::number(in.mesh->GetNumberOfPoints()));
    repl("CELLS", QString::number(in.mesh->GetNumberOfCells()));
    repl("IMPORT_PIPELINE", in.importPipelineText.isEmpty() ? QStringLiteral("-") : in.importPipelineText);

    repl("MESH_CLASS", in.meshClassText.isEmpty() ? QString::fromWCharArray(L"\uFF08\u672A\u5206\u7C7B\uFF09") : in.meshClassText);
    repl("CELLTYPE_STATS_TABLE", in.cellTypeStatsTableMd.isEmpty() ? QString("| - | - | - | - |") : in.cellTypeStatsTableMd);

    repl("METRIC_KEY", in.metricKey);
    repl("METRIC_NAME", in.metricName);
    repl("RANGE_MIN", QString::number(in.qualityRangeMin, 'g', 10));
    repl("RANGE_MAX", QString::number(in.qualityRangeMax, 'g', 10));
    repl("METRIC_SUPPORT", in.metricSupportSummary.isEmpty() ? QStringLiteral("-") : in.metricSupportSummary);

    repl("TEMPLATE_TEXT", in.templateText);
    repl("TEMPLATE_KEY", in.templateKey);
    repl("RULE_TEXT", in.ruleText);
    repl("RULE_KEY", in.ruleKey);
    repl("THRESHOLD", QString::number(in.threshold, 'g', 10));

    repl("COMBO_RULE", in.comboRuleText.isEmpty() ? QString::fromWCharArray(L"\u5355\u6307\u6807") : in.comboRuleText);

    repl("BAD", QString::number(in.badCells));
    repl("TOTAL", QString::number(in.totalCells));
    repl("BAD_RATIO", QString::number(in.badRatioPct, 'f', 2));

    repl("CSV_PATH", in.csvPath.isEmpty() ? QString::fromWCharArray(L"\uFF08\u672A\u5BFC\u51FA\uFF09") : in.csvPath);
    repl("PNG_PATH", in.pngPath.isEmpty() ? QString::fromWCharArray(L"\uFF08\u672A\u5BFC\u51FA\uFF09") : in.pngPath);

    repl("QUALITY_EXPLAIN", qualityExplainZh(in.metricKey));

    // BAD_TABLE (Top 20)
    QString table;
    const int nShow = std::min<int>(20, in.badCells);
    for (int i = 0; i < nShow; ++i)
    {
        const vtkIdType cellId = in.badCellIds[i];
        const int ct = in.mesh->GetCellType(cellId);
        const double qv = in.qualityArray->GetTuple1(cellId);
        table += QString("| %1 | %2 | %3 | %4 |\n")
                     .arg(static_cast<qlonglong>(cellId))
                     .arg(ct)
                     .arg(cellTypeName(ct))
                     .arg(QString::number(qv, 'g', 10));
    }
    if (table.isEmpty())
        table = QString("| - | - | - | - |\n");
    repl("BAD_TABLE", table.trimmed());

    *outMarkdown = tpl;
    return true;
}
} // namespace vtklasttry

