
#include "mainwindow.h"

#include <array>

// VTK初始化宏
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);

// VTK头文件
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCylinderSource.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>

mainwindow::mainwindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
	// 设置背景颜色
	vtkNew<vtkNamedColors> colors;

	// 设置颜色
	std::array<unsigned char, 4> bkg{ {26, 51, 102, 255} };
	colors->SetColor("BkgColor", bkg[0], bkg[1], bkg[2], bkg[3]);

	// 创建八边形圆柱体
	vtkNew<vtkCylinderSource> cylinder;
	cylinder->SetHeight(2.0);
	cylinder->SetRadius(1.0);
	cylinder->SetResolution(8);

	// 创建Mapper
	vtkNew<vtkPolyDataMapper> cylinderMapper;
	cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

	// 创建Actor
	vtkNew<vtkActor> cylinderActor;
	cylinderActor->SetMapper(cylinderMapper);
	cylinderActor->GetProperty()->SetColor(colors->GetColor4d("Tomato").GetData());
	cylinderActor->RotateX(30.0);
	cylinderActor->RotateY(-45.0);

	// 创建Renderer
	vtkNew<vtkRenderer> renderer;
	renderer->AddActor(cylinderActor);
	renderer->SetBackground(colors->GetColor3d("BkgColor").GetData());
	renderer->ResetCamera();
	renderer->GetActiveCamera()->Zoom(1.5);

	// 创建RenderWindow
	vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
	renderWindow->AddRenderer(renderer);

	// 将VTK渲染窗口设置到Qt的OpenGL控件中
	ui.openGLWidget->SetRenderWindow(renderWindow);
}

mainwindow::~mainwindow()
{}


