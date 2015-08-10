#pragma once

#include "win_capture/capture_device.h"

#ifdef WEBCAMSHOW_CPP
#define LINKAGE
#else
#define LINKAGE extern
#endif

struct Global_t
{
	xoSysWnd*		MainWnd;
	CaptureDevice*	Camera;
};
LINKAGE Global_t Global;

void					Util_CameraToCanvas(CaptureDevice* camera, const void* cameraFrame, xoCanvas2D* ccx);
void					Util_LumToCanvas(ccv_dense_matrix_t* lum, xoCanvas2D* ccx, int canvasX = 0, int canvasY = 0);
ccv_dense_matrix_t*		Util_RGB_to_CCV_Lum8(int width, int height, const void* rgb);
void					Util_RGB_to_Lum(int width, int height, const void* rgb, ccv_dense_matrix_t* lum);
ccv_dense_matrix_t* 	Util_Lum_HalfSize_Box(ccv_dense_matrix_t* lum, bool sRGB);
ccv_dense_matrix_t* 	Util_Lum_HalfSize_Box_Until(ccv_dense_matrix_t* lum, bool sRGB, int widthLessThanOrEqualTo);
ccv_dense_matrix_t* 	Util_Clone(ccv_dense_matrix_t* org, int targetType = 0);
void				 	Util_Copy(ccv_dense_matrix_t* dst, ccv_dense_matrix_t* src);

#undef LINKAGE

