#include "pch.h"
#include "Common.h"

#ifdef SX_OPENCV

namespace sx
{

static cv::Ptr<cv::ORB> Orb;
static cv::Ptr<cv::BRISK> Brisk;
static cv::Ptr<cv::MSER> Mser;
static cv::Ptr<cv::GFTTDetector> Gftt;

void ORB_Start(xoDoc* doc)
{
	using namespace cv;

	Orb = cv::ORB::create(500);
	Brisk = cv::BRISK::create();
	Mser = cv::MSER::create();
	Gftt = cv::GFTTDetector::create();
	Gftt->setHarrisDetector(true);

	auto onTimer = [](Image* frame, xoCanvas2D* cx, xoDomNode* label) -> void
	{
		std::vector<KeyPoint> keypoints;
		Mat descriptors;

		auto gray = frame->Clone(ImgFmt::Lum8u);

		Timer timer;
		//Orb->detectAndCompute(frame->ToOpenCV_NoCopy(), Mat(), keypoints, descriptors);
		Brisk->detectAndCompute(frame->ToOpenCV_NoCopy(), Mat(), keypoints, descriptors);
		//Mser->detectAndCompute(gray->ToOpenCV_NoCopy(), Mat(), keypoints, descriptors);
		//Gftt->detectAndCompute(gray->ToOpenCV_NoCopy(), Mat(), keypoints, descriptors);
		label->SetText(xo::fmt("%f", timer.DurationMS()).Z);

		for (auto& kp : keypoints)
			cx->SetPixel(kp.pt.x, kp.pt.y, xoRGBA::RGBA(255, 0, 0, 255));
	
		delete gray;
	};

	Util_SetupTestUI(doc, onTimer);
}

void ORB_End()
{
	Orb.release();
	Brisk.release();
	Mser.release();
	Gftt.release();
}

}

#endif
