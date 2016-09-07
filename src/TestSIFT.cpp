#include "pch.h"
#include "Common.h"

#ifdef SX_CCV

namespace sx
{

void Sift_Start(xoDoc* doc)
{
	auto onTimer = [](Image* frame, xoCanvas2D* cx, xoDomNode* label) -> void
	{
		auto tmp8 = frame->Clone(ImgFmt::Lum8u);
		ccv_dense_matrix_t* lum = tmp8->ToCCV();
		delete tmp8;

		ccv_sift_param_t param;
		param.noctaves = 3;
		param.nlevels = 6;
		param.up2x = 1;
		param.edge_threshold = 10;
		param.norm_threshold = 0;
		param.peak_threshold = 0;

		float scale = 1;

		ccv_array_t* keypoints = nullptr;
		Timer timer;
		ccv_sift(lum, &keypoints, nullptr, 0, param);
		label->SetText(xo::fmt("%f", timer.DurationMS()).Z);

		for (int i = 0; i < keypoints->rnum; i++)
		{
			ccv_keypoint_t* kp = (ccv_keypoint_t*) ccv_array_get(keypoints, i);
			int ix = (int) (kp->x * scale + 0.5f);
			int iy = (int) (kp->y * scale + 0.5f);
			cx->SetPixel(ix, iy, xoRGBA::RGBA(0, 255, 0, 255));
		}
		ccv_array_free(keypoints);
		ccv_matrix_free(lum);
	};

	Util_SetupTestUI(doc, onTimer);
}

void Sift_End()
{
}

}

#endif
