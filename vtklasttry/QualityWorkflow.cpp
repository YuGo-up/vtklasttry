#include "QualityWorkflow.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <vector>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QStatusBar>
#include <QTextEdit>
#include <QWidget>

#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkMeshQuality.h>
#include <vtkNew.h>
#include <vtkScalarBarActor.h>

#include "MeshAnalysis.h"
#include "QualityDefaults.h"
#include "QualityEngine.h"
#include "QualitySupport.h"
#include "UiLog.h"

namespace
{
static QString metricSupportedCellTypesText(const QString& metricKey)
{
    if (metricKey == "aspect")
        return QStringLiteral("VTK_TRIANGLE(5), VTK_QUAD(9), VTK_TETRA(10), VTK_HEXAHEDRON(12)");
    if (metricKey == "skew")
        return QStringLiteral("VTK_TRIANGLE(5), VTK_QUAD(9), VTK_TETRA(10), VTK_HEXAHEDRON(12)");
    if (metricKey == "scaled_jacobian")
        return QStringLiteral("VTK_TRIANGLE(5), VTK_QUAD(9), VTK_TETRA(10), VTK_HEXAHEDRON(12)");
    if (metricKey == "non_ortho")
        return QStringLiteral("VTK_TETRA(10), VTK_HEXAHEDRON(12), VTK_WEDGE(13), VTK_PYRAMID(14)");
    if (metricKey == "jacobian")
        return QStringLiteral("VTK_TRIANGLE(5, 2D:ScaledJacobian), VTK_QUAD(9), VTK_TETRA(10), VTK_HEXAHEDRON(12)");
    if (metricKey == "jacobian_ratio")
        return QStringLiteral("VTK_TRIANGLE(5), VTK_QUAD(9), VTK_TETRA(10), VTK_HEXAHEDRON(12) [Edge ratio]");
    if (metricKey == "min_angle")
        return QStringLiteral("VTK_TRIANGLE(5), VTK_QUAD(9), VTK_TETRA(10) [deg]");
    if (metricKey == "max_angle")
        return QStringLiteral("VTK_TRIANGLE(5), VTK_QUAD(9) [deg]");
    if (metricKey == "min_edge_length")
        return QStringLiteral("LINE/POLY_LINE/TRI/QUAD/TET/HEX/WEDGE/PYRAMID/VOXEL/POLYGON/STRIP...");
    return QStringLiteral("UNKNOWN");
}

} // namespace

namespace vtklasttry
{
bool QualityWorkflow::computeQuality(QWidget* parent,
                                     QTextEdit* logBox,
                                     QStatusBar* statusBar,
                                     const vtklasttry::QualityRequest& req,
                                     vtklasttry::QualityState& state,
                                     const std::function<void(const QString&)>& setBadRuleKey,
                                     const std::function<void(double)>& setThreshold,
                                     const std::function<void()>& applyThresholdTemplateIfNeeded)
{
    if (!state.meshData || !state.qualityArray || !state.qualityCache || !state.lastQualityMetricKey || !state.lastMetricSupportSummary)
        return false;

    vtkDataSet* ds = vtkDataSet::SafeDownCast(*state.meshData);
    if (!ds)
    {
        QMessageBox::warning(
            parent,
            QString::fromWCharArray(L"\u4E0D\u652F\u6301"),
            QString::fromWCharArray(L"\u5F53\u524D\u6570\u636E\u65E0\u6CD5\u4F5C\u4E3A vtkDataSet \u5904\u7406\u3002"));
        return false;
    }

    const QString metricKey = req.metricKey;
    const double thresholdValue = req.thresholdValue;

    // Log metric support info and early-exit if no supported cells exist.
    {
        const int supportedN = vtklasttry::countSupportedCells(ds, metricKey);
        const QString supportedTypes = metricSupportedCellTypesText(metricKey);
        const QString supportBreakdown = vtklasttry::buildMetricSupportSummary(ds, metricKey);
        *state.lastMetricSupportSummary = QString("metric=%1 supported_types=[%2] supported_cells=%3 breakdown={%4}")
                                            .arg(metricKey)
                                            .arg(supportedTypes)
                                            .arg(supportedN)
                                            .arg(supportBreakdown.isEmpty() ? QStringLiteral("-") : supportBreakdown);
        vtklasttry::appendUiLog(logBox, *state.lastMetricSupportSummary);
        if (supportedN <= 0)
        {
            vtklasttry::appendUiLog(logBox, QString::fromWCharArray(
                                                    L"\u63D0\u793A\uFF1A\u5F53\u524D\u7F51\u683C\u4E2D\u6CA1\u6709\u53EF\u8BA1\u7B97\u8BE5\u8D28\u91CF\u6307\u6807\u7684\u5355\u5143\u7C7B\u578B\uFF0C\u8BF7\u5207\u6362\u6307\u6807\u6216\u66F4\u6362\u7F51\u683C\u3002"));
            return false;
        }
    }

    if (metricKey == "min_edge_length")
    {
        vtkSmartPointer<vtkDataArray> arr = vtklasttry::computeMinEdgeLengthQualityArray(ds, nullptr);
        if (!arr)
            return false;

        ds->GetCellData()->AddArray(arr);
        ds->GetCellData()->SetActiveScalars(arr->GetName());
        if (state.scalarBar)
            state.scalarBar->SetTitle("Min edge length");

        *state.meshData = ds;
        *state.qualityArray = ds->GetCellData()->GetArray("Quality");
        if (!(*state.qualityArray)) *state.qualityArray = arr;

        const QString arrayName = QString("Quality_%1").arg(metricKey);
        (*state.qualityArray)->SetName(arrayName.toLocal8Bit().constData());
        ds->GetCellData()->SetActiveScalars((*state.qualityArray)->GetName());
        *state.lastQualityMetricKey = metricKey;
        state.qualityCache->insert(metricKey, *state.qualityArray);

        double range[2] = { 0.0, 1.0 };
        if (!vtklasttry::finiteRange(ds, *state.qualityArray, metricKey, range))
        {
            range[0] = 0.0;
            range[1] = 1.0;
        }
        if (statusBar)
            statusBar->showMessage(
            QString::fromWCharArray(L"\u8D28\u91CF\u8BA1\u7B97\u5B8C\u6210\uFF1Amin=%1, max=%2").arg(range[0]).arg(range[1]));

        {
            const auto rec = vtklasttry::recommendQualityDefaults(metricKey,
                                                                 thresholdValue,
                                                                 vtklasttry::QualityDefaultsContext::MinEdgeLength);
            if (!rec.badRuleKey.isEmpty() && setBadRuleKey)
                setBadRuleKey(rec.badRuleKey);
            if (rec.hasThreshold && setThreshold)
                setThreshold(rec.threshold);
        }

        if (req.templateKey != "custom")
            if (applyThresholdTemplateIfNeeded) applyThresholdTemplateIfNeeded();
        return true;
    }

    vtkNew<vtkMeshQuality> q;
    q->SetInputData(ds);
    q->SetSaveCellQuality(true);

    if (metricKey == "aspect")
    {
        q->SetTriangleQualityMeasureToAspectRatio();
        q->SetQuadQualityMeasureToAspectRatio();
        q->SetTetQualityMeasureToAspectRatio();
        q->SetHexQualityMeasureToEdgeRatio();
        if (state.scalarBar)
            state.scalarBar->SetTitle("Aspect Ratio / Edge Ratio");
    }
    else if (metricKey == "skew")
    {
        q->SetTriangleQualityMeasureToShapeAndSize();
        q->SetQuadQualityMeasureToSkew();
        q->SetTetQualityMeasureToScaledJacobian();
        q->SetHexQualityMeasureToSkew();
        if (state.scalarBar)
            state.scalarBar->SetTitle("Skew (Quad/Hex) / Shape&Size (Tri) / ScaledJacobian (Tet)");
    }
    else if (metricKey == "scaled_jacobian")
    {
        q->SetTriangleQualityMeasureToScaledJacobian();
        q->SetQuadQualityMeasureToScaledJacobian();
        q->SetTetQualityMeasureToScaledJacobian();
        q->SetHexQualityMeasureToScaledJacobian();
        if (state.scalarBar)
            state.scalarBar->SetTitle("Scaled Jacobian");
    }
    else if (metricKey == "jacobian")
    {
        q->SetTriangleQualityMeasureToScaledJacobian();
        q->SetQuadQualityMeasureToJacobian();
        q->SetTetQualityMeasureToJacobian();
        q->SetHexQualityMeasureToJacobian();
        if (state.scalarBar)
            state.scalarBar->SetTitle("Jacobian (Tri:ScaledJac)");
    }
    else if (metricKey == "jacobian_ratio")
    {
        q->SetTriangleQualityMeasureToEdgeRatio();
        q->SetQuadQualityMeasureToEdgeRatio();
        q->SetTetQualityMeasureToEdgeRatio();
        q->SetHexQualityMeasureToEdgeRatio();
        if (state.scalarBar)
            state.scalarBar->SetTitle("Jacobian ratio ~ Edge ratio");
    }
    else if (metricKey == "min_angle")
    {
        q->SetTriangleQualityMeasureToMinAngle();
        q->SetQuadQualityMeasureToMinAngle();
        q->SetTetQualityMeasureToMinAngle();
        if (state.scalarBar)
            state.scalarBar->SetTitle("Min angle (deg)");
    }
    else if (metricKey == "max_angle")
    {
        q->SetTriangleQualityMeasureToMaxAngle();
        q->SetQuadQualityMeasureToMaxAngle();
        if (state.scalarBar)
            state.scalarBar->SetTitle("Max angle (deg)");
    }
    else if (metricKey == "non_ortho")
    {
        vtkSmartPointer<vtkDataArray> arr = vtklasttry::computeNonOrthoQualityArray(ds, nullptr);
        if (!arr)
            return false;

        ds->GetCellData()->AddArray(arr);
        ds->GetCellData()->SetActiveScalars(arr->GetName());
        if (state.scalarBar)
            state.scalarBar->SetTitle("Non-orthogonality (deg)");

        vtkDataSet* out = ds;
        *state.meshData = out;
        *state.qualityArray = out->GetCellData()->GetArray("Quality");
        if (!(*state.qualityArray))
            *state.qualityArray = arr;

        const QString arrayName = QString("Quality_%1").arg(metricKey);
        (*state.qualityArray)->SetName(arrayName.toLocal8Bit().constData());
        out->GetCellData()->SetActiveScalars((*state.qualityArray)->GetName());
        *state.lastQualityMetricKey = metricKey;
        state.qualityCache->insert(metricKey, *state.qualityArray);

        double range[2] = { 0.0, 1.0 };
        if (!vtklasttry::finiteRange(out, *state.qualityArray, metricKey, range))
        {
            range[0] = 0.0;
            range[1] = 1.0;
        }
        if (statusBar)
            statusBar->showMessage(
                QString::fromWCharArray(L"\u8D28\u91CF\u8BA1\u7B97\u5B8C\u6210\uFF1Amin=%1, max=%2").arg(range[0]).arg(range[1]));

        {
            const auto rec = vtklasttry::recommendQualityDefaults(metricKey,
                                                                 thresholdValue,
                                                                 vtklasttry::QualityDefaultsContext::NonOrtho);
            if (!rec.badRuleKey.isEmpty() && setBadRuleKey)
                setBadRuleKey(rec.badRuleKey);
            if (rec.hasThreshold && setThreshold)
                setThreshold(rec.threshold);
        }

        if (req.templateKey != "custom")
            if (applyThresholdTemplateIfNeeded) applyThresholdTemplateIfNeeded();
        return true;
    }
    else
    {
        QMessageBox::warning(
            parent,
            QString::fromWCharArray(L"\u4E0D\u652F\u6301"),
            QString::fromWCharArray(L"\u672A\u77E5\u8D28\u91CF\u6307\u6807\u3002"));
        return false;
    }

    q->Update();

    vtkDataSet* out = q->GetOutput();
    if (!out || !out->GetCellData() || !out->GetCellData()->GetArray("Quality"))
    {
        QMessageBox::warning(
            parent,
            QString::fromWCharArray(L"\u8BA1\u7B97\u5931\u8D25"),
            QString::fromWCharArray(L"\u672A\u80FD\u751F\u6210\u8D28\u91CF\u6570\u7EC4\uFF08Quality\uFF09\u3002"));
        return false;
    }

    const QString arrayName = QString("Quality_%1").arg(metricKey);
    vtkDataArray* qualityRaw = out->GetCellData()->GetArray("Quality");
    qualityRaw->SetName(arrayName.toLocal8Bit().constData());

    *state.meshData = out;
    *state.qualityArray = qualityRaw;
    out->GetCellData()->SetActiveScalars(qualityRaw->GetName());
    *state.lastQualityMetricKey = metricKey;
    state.qualityCache->insert(metricKey, *state.qualityArray);

    {
        const vtkIdType nCells = out->GetNumberOfCells();
        const double nanv = std::numeric_limits<double>::quiet_NaN();
        for (vtkIdType i = 0; i < nCells; ++i)
        {
            const int ct = out->GetCellType(i);
            if (!vtklasttry::isMetricSupportedCell(metricKey, ct))
                (*state.qualityArray)->SetTuple1(i, nanv);
        }
    }

    double range[2] = { 0.0, 1.0 };
    if (!vtklasttry::finiteRange(out, *state.qualityArray, metricKey, range))
    {
        range[0] = 0.0;
        range[1] = 1.0;
    }
    if (statusBar)
        statusBar->showMessage(
            QString::fromWCharArray(L"\u8D28\u91CF\u8BA1\u7B97\u5B8C\u6210\uFF1Amin=%1, max=%2").arg(range[0]).arg(range[1]));

    {
        const auto rec = vtklasttry::recommendQualityDefaults(metricKey,
                                                             thresholdValue,
                                                             vtklasttry::QualityDefaultsContext::Generic);
        if (!rec.badRuleKey.isEmpty() && setBadRuleKey)
            setBadRuleKey(rec.badRuleKey);
        if (rec.hasThreshold && setThreshold)
            setThreshold(rec.threshold);
    }

    if (req.templateKey != "custom")
        if (applyThresholdTemplateIfNeeded) applyThresholdTemplateIfNeeded();
    return true;
}

} // namespace vtklasttry

