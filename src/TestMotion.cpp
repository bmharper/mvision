#include "pch.h"
#include "Common.h"

static struct Motion_t
{
	xoDomCanvas* Canvas1;
	xoDomCanvas* Canvas2;
	xoDomCanvas* Canvas3;
} Motion;

static bool OnTimer(const xoEvent& ev)
{
	auto cam = Global.Camera;
	if (cam)
	{
		void* cameraFrame = cam->GetNextFrame();
		if (cameraFrame)
		{
			auto c1 = Motion.Canvas1->GetCanvas2D();
			Util_CameraToCanvas(cam, cameraFrame, c1);
			Motion.Canvas1->ReleaseCanvas(c1);

			auto c2 = Motion.Canvas2->GetCanvas2D();
			auto c3 = Motion.Canvas3->GetCanvas2D();
			ccv_dense_matrix_t* lum = Util_RGB_to_CCV_Lum8(cam->GetWidth(), cam->GetHeight(), cameraFrame);
			ccv_dense_matrix_t* half = Util_Lum_HalfSize_Cheap(lum);
			ccv_dense_matrix_t* quarter = Util_Lum_HalfSize_Cheap(half);
			Util_LumToCanvas(half, c2);
			Util_LumToCanvas(quarter, c3);
			ccv_matrix_free(lum);
			ccv_matrix_free(half);
			ccv_matrix_free(quarter);
			Motion.Canvas2->ReleaseCanvas(c2);
			Motion.Canvas3->ReleaseCanvas(c3);

			free(cameraFrame);
		}
	}
	return true;
}

void Motion_Start(xoDoc* doc)
{
	xoDomCanvas* c1 = doc->Root.AddCanvas();
	xoDomCanvas* c2 = doc->Root.AddCanvas();
	xoDomCanvas* c3 = doc->Root.AddCanvas();
	Motion.Canvas1 = c1;
	Motion.Canvas2 = c2;
	Motion.Canvas3 = c3;
	c1->SetSize(640, 480);
	c1->StyleParse("background: #ffaf");
	c2->SetSize(320, 240);
	c2->StyleParse("background: #afff");
	c3->SetSize(160, 120);
	c3->StyleParse("background: #faff");
	doc->Root.OnTimer(OnTimer, nullptr, 15);
}

void Motion_End()
{

}


