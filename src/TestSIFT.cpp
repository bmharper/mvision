#include "pch.h"
#include "Common.h"

#ifdef SX_CCV

static struct Sift_t
{
	xoDomCanvas*	Canvas;
	xoDomNode*		Txt;
} Sift;

static bool OnTimer(const xoEvent& ev)
{
	auto cam = Global.Camera;
	if (cam)
	{
		void* cameraFrame = cam->NextFrame();
		if (cameraFrame)
		{
			auto c1 = Sift.Canvas->GetCanvas2D();
			Util_CameraToCanvas(cam, cameraFrame, c1);

			ccv_dense_matrix_t* lum = Util_RGB_to_CCV_Lum8(cam->Width(), cam->Height(), cameraFrame);
			ccv_dense_matrix_t* half = Util_Lum_HalfSize_Box(lum, false);
			ccv_dense_matrix_t* quarter = Util_Lum_HalfSize_Box(half, false);

			ccv_sift_param_t param;
			param.noctaves = 3;
			param.nlevels = 6;
			param.up2x = 1;
			param.edge_threshold = 10;
			param.norm_threshold = 0;
			param.peak_threshold = 0;

			float scale = 1;

			ccv_array_t* keypoints = nullptr;
			auto start = std::chrono::high_resolution_clock::now();
			ccv_sift(lum, &keypoints, nullptr, 0, param);
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
			char buf[100];
			sprintf(buf, "%f", (double) duration.count());
			Sift.Txt->SetText(buf);

			for (int i = 0; i < keypoints->rnum; i++)
			{
				ccv_keypoint_t* kp = (ccv_keypoint_t*) ccv_array_get(keypoints, i);
				int ix = (int) (kp->x * scale + 0.5f);
				int iy = (int) (kp->y * scale + 0.5f);
				c1->SetPixel(ix, iy, xoRGBA::RGBA(0, 255, 0, 255));
			}
			ccv_array_free(keypoints);
			ccv_matrix_free(quarter);
			ccv_matrix_free(half);
			ccv_matrix_free(lum);

			Sift.Canvas->ReleaseCanvas(c1);
			free(cameraFrame);
		}
	}
	return true;
}

void Sift_Start(xoDoc* doc)
{
	xoDomCanvas* c1 = doc->Root.AddCanvas();
	xoDomNode* txt = doc->Root.AddNode(xoTagDiv);
	Sift.Canvas = c1;
	Sift.Txt = txt;
	c1->SetSize(640, 480);
	c1->StyleParse("background: #ffaf");
	doc->Root.OnTimer(OnTimer, nullptr, 15);
}

void Sift_End()
{
}

#endif
