#include "pch.h"
#include "Common.h"

#ifdef SX_OPENCV

namespace sx
{

static cv::Ptr<cv::BackgroundSubtractor> BGSub;

void BackgroundSubtractor_Start(xoDoc* doc)
{
	using namespace cv;

	//BGSub = createBackgroundSubtractorKNN().dynamicCast<BackgroundSubtractor>();
	BGSub = createBackgroundSubtractorMOG2(500, 8, false).dynamicCast<BackgroundSubtractor>();

	auto onTimer = [](Image* frame, xoCanvas2D* cx, xoDomNode* label) -> void
	{
		Mat fgmask;
		Timer timer;
		BGSub->apply(frame->ToOpenCV_NoCopy(), fgmask);
		label->SetText(fmt("%f", timer.DurationMS()).Z);

		for (int y = 0; y < fgmask.rows; y++)
		{
			uint8* mask = fgmask.ptr(y);
			xoRGBA* res = (xoRGBA*) cx->RowPtr(y);
			for (int x = 0; x < fgmask.cols; x++)
			{
				if (mask[x])
					res[x].r = 255;
			}
		}
	};

	Util_SetupTestUI(doc, onTimer);
}

void BackgroundSubtractor_End()
{
	BGSub.release();
}

}

#endif
