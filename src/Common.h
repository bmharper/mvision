#pragma once

#include "image/image.h"
#include "cameras/Cameras.h"
#include "util/Error.h"

// SX_LINKAGE is undef'ed at the end of this header
#ifdef WEBCAMSHOW_CPP
#define SX_LINKAGE
#else
#define SX_LINKAGE extern
#endif

namespace sx
{

class Live555Environment;

struct Global_t
{
	xoSysWnd*				MainWnd = nullptr;
	ICamera*				Camera = nullptr;
#ifdef SX_LIVE555
	Live555Environment*		Live555Env = nullptr;
#endif
	microlog::Logger		Log;
};
SX_LINKAGE Global_t Global;

microlog::Logger* Log();

void		Util_ImageToCanvas(const Image* img, xoCanvas2D* ccx);
void		Util_LumToCanvas(const Image* lum, xoCanvas2D* ccx, int canvasX = 0, int canvasY = 0, int scale = 1);

void		Util_SetupTestUI(xoDoc* doc, std::function<void(Image* frame, xoCanvas2D* cx, xoDomNode* label)> onTimer);

}

#undef SX_LINKAGE

