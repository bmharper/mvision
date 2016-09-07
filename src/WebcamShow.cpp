#include "pch.h"
#include "cameras/win_capture/capture_device.h"
#include "cameras/OpenCV.h"
#include "cameras/live555.h"

#ifdef SX_CCV
// For libccv
#include <Delayimp.h>
#endif

// State
#define WEBCAMSHOW_CPP
#include "Common.h"

namespace sx
{

// Modules
struct Module
{
	void (*Start)(xoDoc* doc);
	void (*End)();
};
#ifdef SX_CCV
void TLD_Start(xoDoc* doc);
void TLD_End();
void Sift_Start(xoDoc* doc);
void Sift_End();
static Module M_TLD = { TLD_Start, TLD_End };
static Module M_Sift = { Sift_Start, Sift_End };
#endif

#ifdef SX_OPENCV
void ORB_Start(xoDoc* doc);
void ORB_End();
void BackgroundSubtractor_Start(xoDoc* doc);
void BackgroundSubtractor_End();
static Module M_ORB = { ORB_Start, ORB_End };
static Module M_BackgroundSubtractor = { BackgroundSubtractor_Start, BackgroundSubtractor_End };
#endif

void Motion_Start(xoDoc* doc);
void Motion_End();
static Module M_Motion = { Motion_Start, Motion_End };

//static Module *Mod = &M_TLD;
//static Module *Mod = &M_Sift;
static Module *Mod = &M_Motion;
//static Module *Mod = &M_ORB;
//static Module *Mod = &M_BackgroundSubtractor;

// Helpers
static void LoadCCV();
static void InitializeLogs();
static void InitializeCamera();

}

void xoMain(xoMainEvent ev)
{
	using namespace sx;

	switch (ev)
	{
	case xoMainEventInit:
		LoadCCV();

		InitializeLogs();

		Global.MainWnd = xoSysWnd::CreateWithDoc();
		
		Mod->Start(Global.MainWnd->Doc());

		Global.MainWnd->Show();
		InitializeCamera();
		break;
	case xoMainEventShutdown:
		//Mod->End();
		delete Global.Camera;
		Global.Camera = nullptr;
		//SafeRelease(&Global.Camera);
		MFShutdown();
		CoUninitialize();
		delete Global.MainWnd;
		Global.MainWnd = nullptr;
		//Global.Log = nullptr;
		break;
	}
}

namespace sx
{

static void LoadCCV()
{
#ifdef SX_CCV
	//HRESULT res = __HrLoadAllImportsForDll("libccv.dll");
	// See if *any* code from ccv will execute. I'm getting strange failures on my Win10 machine
	//ccv_matrix_t* mat = ccv_dense_matrix_new(16, 16, CCV_8U | CCV_C1, nullptr, 0);
	//if (mat)
	//	ccv_matrix_free(mat);
#endif
}

static void InitializeLogs()
{
	Global.Log.Level = microlog::Level::Debug;
	Global.Log.SetFile("c:/temp/mvision.log");
	Global.Log.Info("Hello");
}

static void InitializeCamera()
{
	if (false)
	{
		WinCaptureDevice* cam = new WinCaptureDevice();
		std::string error;
		if (cam->InitializeFirst(error))
			Global.Camera = cam;
		else
			delete cam;
	}

#ifdef SX_OPENCV
	if (!Global.Camera && false)
	{
		// This doesn't work on Windows - I get corruption from my HikVision camera.
		OpenCVCamera* cam = new OpenCVCamera();
		if (cam->Open("rtsp://admin:PASSWORD@192.168.1.104:554?tcp"))
			Global.Camera = cam;
		else
			delete cam;
	}
#endif

#ifdef SX_LIVE555
	if (!Global.Camera && true)
	{
		Live555Environment::Create();
		Live555Camera* cam = new Live555Camera();
		if (cam->Open("rtsp://admin:PASSWORD@192.168.1.104:554"))
			Global.Camera = cam;
		else
			delete cam;
	}
#endif
}


}