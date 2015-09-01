#pragma once

#include "image/image.h"
#include "cameras/Cameras.h"
#include "cameras/win_capture/capture_device.h"

#ifdef WEBCAMSHOW_CPP
#define LINKAGE
#else
#define LINKAGE extern
#endif

namespace sx
{

struct Global_t
{
	xoSysWnd*		MainWnd;
	CaptureDevice*	Camera;
};
LINKAGE Global_t Global;

void		Util_ImageToCanvas(const Image* img, xoCanvas2D* ccx);
void		Util_LumToCanvas(const Image* lum, xoCanvas2D* ccx, int canvasX = 0, int canvasY = 0, int scale = 1);
Image* 		Util_Lum_HalfSize_Box(Image* lum, bool sRGB);
Image* 		Util_Lum_HalfSize_Box_Until(Image* lum, bool sRGB, int widthLessThanOrEqualTo);
Image* 		Util_Clone(Image* org, ImgFmt targetFmt = ImgFmt::Null);

//void					Util_RGB_to_Lum(int width, int height, const void* rgb, ccv_dense_matrix_t* lum);
//ccv_dense_matrix_t*		Util_RGB_to_CCV_Lum8(int width, int height, const void* rgb);

}

#undef LINKAGE

