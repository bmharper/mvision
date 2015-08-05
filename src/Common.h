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
void					Util_LumToCanvas(ccv_dense_matrix_t* lum, xoCanvas2D* ccx);
ccv_dense_matrix_t*		Util_RGB_to_CCV_Lum8(int width, int height, const void* rgb);
void					Util_RGB_to_Lum(int width, int height, const void* rgb, ccv_dense_matrix_t* lum);
ccv_dense_matrix_t* 	Util_Lum_HalfSize_Cheap(ccv_dense_matrix_t* lum);
ccv_dense_matrix_t* 	Util_Lum_HalfSize_Cheap_Linear(ccv_dense_matrix_t* lum);

#undef LINKAGE

