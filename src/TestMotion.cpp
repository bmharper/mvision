#include "pch.h"
#include "Common.h"
#include "MotionDetect.h"

namespace sx
{

static struct Motion_t
{
	xoDomCanvas*	Canvas1;
	xoDomCanvas*	Canvas2;
	xoDomNode*		Stats;
	MotionDetector*	Detector;
} Motion;

static void UpdateStats()
{
	auto msg = fmt("Noise: %.3f. %s", Motion.Detector->Noise, Motion.Detector->IsMotion ? "MOVE" : "--");
	Motion.Stats->SetText(msg.Z);
}

static bool OnTimer(const xoEvent& ev)
{
	auto cam = Global.Camera;
	if (cam)
	{
		Image* cameraFrame = cam->NextFrame();
		if (cameraFrame)
		{
			auto c1 = Motion.Canvas1->GetCanvas2D();
			Util_ImageToCanvas(cameraFrame, c1);
			Motion.Canvas1->ReleaseCanvas(c1);

			auto c2 = Motion.Canvas2->GetCanvas2D();
			Image* lum = cameraFrame->Clone(ImgFmt::Lum8u);
			Motion.Detector->Frame(lum);
			if (Motion.Detector->Stable)
				Util_LumToCanvas(Motion.Detector->Stable, c2, 0, 0, 4);
			if (Motion.Detector->DebugImage)
				Util_LumToCanvas(Motion.Detector->DebugImage, c2, 80, 0, 4);
			delete lum;
			Motion.Canvas2->ReleaseCanvas(c2);

			delete cameraFrame;

			UpdateStats();
		}
	}
	return true;
}

void Motion_Start(xoDoc* doc)
{
	Motion.Detector = new MotionDetector();
	xoDomCanvas* c1 = doc->Root.AddCanvas();
	xoDomCanvas* c2 = doc->Root.AddCanvas();
	xoDomNode* stats = doc->Root.AddNode(xoTagDiv);
	Motion.Canvas1 = c1;
	Motion.Canvas2 = c2;
	Motion.Stats = stats;
	c1->SetSize(640, 480);
	c1->StyleParse("background: #ffaf");
	c2->SetSize(320, 240);
	c2->StyleParse("background: #afff");
	stats->StyleParse("break: before");
	doc->Root.OnTimer(OnTimer, nullptr, 15);
}

void Motion_End()
{
	delete Motion.Detector;
}

}