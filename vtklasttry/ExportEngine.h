#pragma once

#include <limits>

#include <QHash>
#include <QString>
#include <QVector>

#include <vtkSmartPointer.h>

class vtkDataArray;
class vtkDataSet;
class vtkGenericOpenGLRenderWindow;
using vtkIdType = long long;

#include "RuleEngine.h"

namespace vtklasttry
{
struct ExportCsvInput
{
    QString sourcePath;
    QString displayMetricKey;
    QString displayMetricName;

    bool comboEnabled = false;
    QString ruleKey;
    QString ruleText;
    double threshold = std::numeric_limits<double>::quiet_NaN();

    QString comboLogic;   // "and"/"or"
    QString comboRuleText;
    QVector<RuleCondition> comboRules;

    vtkSmartPointer<vtkDataSet> mesh;
    vtkSmartPointer<vtkDataArray> displayQualityArray; // the array currently used for coloring (single metric)
    QHash<QString, vtkSmartPointer<vtkDataArray>> qualityCache; // metric_key -> array (for q1..qN)
    QVector<vtkIdType> badCellIds;
};

struct ExportReportInput
{
    QString metricKey;
    QString metricName;
    QString ruleKey;
    QString ruleText;
    QString templateKey;
    QString templateText;
    double threshold = std::numeric_limits<double>::quiet_NaN();

    QString sourcePath;
    QString importPipelineText;
    QString meshClassText;
    QString cellTypeStatsTableMd;
    QString metricSupportSummary;

    QString comboRuleText; // already formatted

    QString csvPath;
    QString pngPath;

    vtkSmartPointer<vtkDataSet> mesh;
    vtkSmartPointer<vtkDataArray> qualityArray;
    QVector<vtkIdType> badCellIds;

    int totalCells = 0;
    int badCells = 0;
    double badRatioPct = 0.0;

    double qualityRangeMin = std::numeric_limits<double>::quiet_NaN();
    double qualityRangeMax = std::numeric_limits<double>::quiet_NaN();

    QString templateMarkdown; // loaded from resource by caller
};

bool exportBadCellsCsv(const QString& path, const ExportCsvInput& in, QString* outError);
bool exportScreenshotPng(vtkSmartPointer<vtkGenericOpenGLRenderWindow> rw, const QString& path, QString* outError);
bool buildReportMarkdown(const ExportReportInput& in, QString* outMarkdown, QString* outError);
bool writeUtf8TextFile(const QString& path, const QString& content, QString* outError);
} // namespace vtklasttry

