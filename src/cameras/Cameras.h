#pragma once

namespace sx
{

class Image;

class ICamera
{
public:
	virtual ~ICamera() {}

	virtual int		Width() = 0;
	virtual int		Height() = 0;

	// The caller must delete this image when done with it
	virtual Image*	NextFrame() = 0;
};

}