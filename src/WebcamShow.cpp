#include "pch.h"

// State
#define WEBCAMSHOW_CPP
#include "Common.h"

// Modules
struct Module
{
	void (*Start)(xoDoc* doc);
	void (*End)();
};
void TLD_Start(xoDoc* doc);
void TLD_End();
void Motion_Start(xoDoc* doc);
void Motion_End();
static Module M_TLD = { TLD_Start, TLD_End };
static Module M_Motion = { Motion_Start, Motion_End };
//static Module *Mod = &M_TLD;
static Module *Mod = &M_Motion;

// Helpers
static void InitializeCamera();

void xoMain(xoMainEvent ev)
{
	switch (ev)
	{
	case xoMainEventInit:
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
