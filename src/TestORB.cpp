#include "pch.h"
#include "Common.h"

#ifdef SX_OPENCV

namespace sx
{

static cv::Ptr<cv::ORB> Orb;

void ORB_Start(xoDoc* doc)
{
	using namespace cv;

	Orb = cv::ORB::create(10000);

	auto onTimer = [](Image* frame, xoCanvas2D* cx, xoDomNode* label) -> void
	{
		std::vector<KeyPoint> keypoints;
		Mat descriptors;

		Timer timer;
		Orb->detectAndCompute(frame->ToOpenCV_NoCopy(), Mat(), keypoints, descriptors);
		label->SetText(fmt("%f", timer.DurationMS()).Z);

		for (auto& kp : keypoints)
			cx->SetPixel(kp.pt.x, kp.pt.y, xoRGBA::RGBA(255, 0, 0, 255));
	};

	Util_SetupTestUI(doc, onTimer);
}

void ORB_End()
{
	Orb.release();
}

}

#endif
