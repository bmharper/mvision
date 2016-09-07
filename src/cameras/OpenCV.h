#pragma once

#ifdef SX_OPENCV

#include "Cameras.h"

namespace sx
{

class OpenCVCamera : public ICamera
{
public:
	cv::VideoCapture VCap;

	bool	Open(std::string url);

	int		Width() override;
	int		Height() override;
	Image*	NextFrame() override;

};

}

#endif