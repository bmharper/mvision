#pragma once

#include "image/image.h"
#include "cameras/Cameras.h"
#include "cameras/win_capture/capture_device.h"

#ifdef WEBCAMSHOW_CPP
#define SX_LINKAGE
#else
#define SX_LINKAGE extern
#endif

namespace sx
{

struct Global_t
{
	xoSysWnd*		MainWnd;
	CaptureDevice*	Camera;
};
SX_LINKAGE Global_t Global;

struct Timer
{
	std::chrono::system_clock::time_point	Start;
	Timer()
	{
		Start = std::chrono::high_resolution_clock::now();
	}
	double DurationMS() const
	{
		return (double) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - Start).count();
	}
};

template<typename T> T Clamp(T v, T low, T high) { return v < low ? low : (v > high ? high : v); }

void		Util_ImageToCanvas(const Image* img, xoCanvas2D* ccx);
void		Util_LumToCanvas(const Image* lum, xoCanvas2D* ccx, int canvasX = 0, int canvasY = 0, int scale = 1);

void		Util_SetupTestUI(xoDoc* doc, std::function<void(Image* frame, xoCanvas2D* cx, xoDomNode* label)> onTimer);

}

#undef SX_LINKAGE

