#include "pch.h"

// For libccv
#include <Delayimp.h>

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

void Motion_Start(xoDoc* doc);
void Motion_End();
static Module M_Motion = { Motion_Start, Motion_End };

//static Module *Mod = &M_TLD;
//static Module *Mod = &M_Sift;
static Module *Mod = &M_Motion;

// Helpers
static void InitializeCamera();

}

void xoMain(xoMainEvent ev)
{
	using namespace sx;

	switch (ev)
	{
	case xoMainEventInit:
#ifdef SX_CCV
		{
			HRESULT res = __HrLoadAllImportsForDll("libccv.dll");
			ccv_dense_matrix_new(16, 16, CCV_8U | CCV_C1, nullptr, 0);
		}
#endif
		Global.MainWnd = xoSysWnd::CreateWithDoc();
		
		Mod->Start(Global.MainWnd->Doc());

		Global.MainWnd->Show();
		InitializeCamera();
		break;
	case xoMainEventShutdown:
		Mod->End();
		if (Global.Camera)
			Global.Camera->Close();
		SafeRelease(&Global.Camera);
		MFShutdown();
		CoUninitialize();
		delete Global.MainWnd;
		Global.MainWnd = NULL;
		break;
	}
}

namespace sx
{

static void InitializeCamera()
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	Global.Camera = new CaptureDevice();
	std::string error;
	if (!Global.Camera->InitializeFirst(error))
	{
		delete Global.Camera;
		Global.Camera = NULL;
	}
}

}