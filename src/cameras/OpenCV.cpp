#include "pch.h" 
#include "OpenCV.h" 
#include "../image/image.h"

#ifdef SX_OPENCV

namespace sx
{

bool OpenCVCamera::Open(std::string url)
{
	return VCap.open(url);
}

int OpenCVCamera::Width()
{
	return 0;
}

int OpenCVCamera::Height()
{
	return 0;
}

Image* OpenCVCamera::NextFrame()
{
	cv::Mat img;
	if (!VCap.read(img))
		return nullptr;

	return Image::FromOpenCV(img);
}

}

#endif