#include "VTKPipeline.h"

#include <array>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGeometryFilter.h>
#include <vtkLookupTable.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkThreshold.h>
#include <vtkUnstructuredGrid.h>

VTKPipeline::VTKPipeline()
{
    m_renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderWindow->AddRenderer(m_renderer);

    setupDefaultBackground();
}

vtkGenericOpenGLRenderWindow* VTKPipeline::renderWindow() const
{
    return m_renderWindow.GetPointer();
}

void VTKPipeline::setMesh(vtkSmartPointer<vtkPolyData> polyData)
{
    m_polyData = polyData;
    ensureActor();

    if (m_mapper && m_polyData)
    {
        m_mapper->SetInputData(m_polyData);
        m_renderer->ResetCamera();
        m_renderer->GetActiveCamera()->Zoom(1.5);
        updateBadCellsHighlight();
        m_renderWindow->Render();
    }
}

void VTKPipeline::applyQualityScalar(const std::string& arrayName,
                                     double minVal,
                                     double maxVal)
{
    if (!m_mapper || !m_polyData)
    {
        return;
    }

    ensureActor();

    if (!m_lut)
    {
        m_lut = vtkSmartPointer<vtkLookupTable>::New();
        m_lut->SetNumberOfTableValues(256);
        m_lut->Build();

        // 简单的蓝-绿-红渐变
        for (int i = 0; i < 256; ++i)
        {
            double t = static_cast<double>(i) / 255.0;
            double r = t;
            double g = 1.0 - std::fabs(t - 0.5) * 2.0;
            double b = 1.0 - t;
            if (g < 0.0) g = 0.0;
            m_lut->SetTableValue(i, r, g, b, 1.0);
        }
    }

    m_mapper->SetLookupTable(m_lut);
    m_mapper->SetScalarModeToUseCellData();
    m_mapper->SelectColorArray(arrayName.c_str());
    m_mapper->SetScalarRange(minVal, maxVal);
    m_mapper->ScalarVisibilityOn();

    m_renderer->ResetCameraClippingRange();
    m_renderWindow->Render();
}

void VTKPipeline::updateBadCellsHighlight()
{
    if (!m_polyData)
    {
        if (m_badCellActor)
        {
            m_badCellActor->VisibilityOff();
            m_renderWindow->Render();
        }
        return;
    }

    vtkCellData* cellData = m_polyData->GetCellData();
    vtkDataArray* badArr = cellData ? cellData->GetArray("BadCell") : nullptr;
    if (!badArr)
    {
        if (m_badCellActor)
        {
            m_badCellActor->VisibilityOff();
            m_renderWindow->Render();
        }
        return;
    }

    vtkNew<vtkThreshold> threshold;
    threshold->SetInputData(m_polyData);
    threshold->SetInputArrayToProcess(0, 0, 0,
        vtkDataObject::FIELD_ASSOCIATION_CELLS, "BadCell");
    threshold->ThresholdBetween(0.5, 1.5);
    threshold->Update();

    vtkUnstructuredGrid* out = threshold->GetOutput();
    if (!out || out->GetNumberOfCells() == 0)
    {
        if (m_badCellActor)
        {
            m_badCellActor->VisibilityOff();
            m_renderWindow->Render();
        }
        return;
    }

    vtkNew<vtkGeometryFilter> geom;
    geom->SetInputConnection(threshold->GetOutputPort());
    geom->Update();

    if (!m_badCellActor)
    {
        vtkNew<vtkPolyDataMapper> badMapper;
        m_badCellActor = vtkSmartPointer<vtkActor>::New();
        m_badCellActor->SetMapper(badMapper);
        m_badCellActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
        m_badCellActor->GetProperty()->EdgeVisibilityOn();
        m_badCellActor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
        m_renderer->AddActor(m_badCellActor);
    }

    vtkPolyDataMapper* badMapper = vtkPolyDataMapper::SafeDownCast(m_badCellActor->GetMapper());
    if (badMapper)
    {
        badMapper->SetInputData(geom->GetOutput());
        m_badCellActor->VisibilityOn();
    }
    m_renderWindow->Render();
}

int64_t VTKPipeline::pickCell(int displayX, int displayY)
{
    if (!m_renderer || !m_renderWindow)
        return -1;

    vtkNew<vtkCellPicker> picker;
    picker->SetTolerance(0.001);
    picker->AddPickList(m_actor);
    picker->PickFromListOn();
    int ok = picker->Pick(displayX, displayY, 0, m_renderer);
    if (ok)
    {
        vtkIdType cellId = picker->GetCellId();
        if (cellId >= 0)
            return static_cast<int64_t>(cellId);
    }
    return -1;
}

void VTKPipeline::resetCamera()
{
    if (m_renderer)
    {
        m_renderer->ResetCamera();
        m_renderer->GetActiveCamera()->Zoom(1.5);
        m_renderWindow->Render();
    }
}

void VTKPipeline::ensureActor()
{
    if (!m_mapper)
    {
        m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    }
    if (!m_actor)
    {
        m_actor = vtkSmartPointer<vtkActor>::New();
        m_actor->SetMapper(m_mapper);
        // show black edges for all mesh cells
        m_actor->GetProperty()->EdgeVisibilityOn();
        m_actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
        m_actor->GetProperty()->SetLineWidth(1.0);
        m_renderer->AddActor(m_actor);
    }
}

void VTKPipeline::setupDefaultBackground()
{
    vtkNew<vtkNamedColors> colors;
    std::array<unsigned char, 4> bkg{ {26, 51, 102, 255} };
    colors->SetColor("BkgColor", bkg[0], bkg[1], bkg[2], bkg[3]);
    m_renderer->SetBackground(colors->GetColor3d("BkgColor").GetData());
}

