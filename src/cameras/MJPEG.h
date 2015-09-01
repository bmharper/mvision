#pragma once

#include "Cameras.h"

namespace sx
{

class MJPEGCamera : public ICamera
{
public:

	int		Width() override;
	int		Height() override;
	Image*	NextFrame() override;

};

}