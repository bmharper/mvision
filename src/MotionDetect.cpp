#include "pch.h"
#include "MotionDetect.h"
#include "Common.h"

namespace sx
{

MotionDetector::MotionDetector()
{
}

MotionDetector::~MotionDetector()
{
	delete Prev;
	delete Stable;
	delete DebugImage;
}

void MotionDetector::Frame(Image* frame)
{
	assert(frame->Fmt == ImgFmt::Lum8u);

	const int width = 20;

	Image* myframe = frame;
	if (frame->Width > width)
		myframe = frame->HalfSize_Box_Until(false, width);

	if (Prev)
	{
		float noise = ComputeNoise(Prev, myframe);
		float a = std::min(NFrames, (int64_t) 500);
		float b = 1;
		Noise = (a * Noise + b * noise) / (a + b);
	}

	if (Stable)
		ComputePixelsInMotion(myframe);

	MergeIntoPrev(myframe);

	if (!Prev)
		Prev = myframe->Clone();
	else
		myframe->CopyTo(Prev);

	if (myframe != frame)
		delete myframe;

	NFrames++;
}

float MotionDetector::ComputeNoise(Image* a, Image* b)
{
	// uber-simple: just treat the entire scene as noise.
	assert(a->Width == b->Width && a->Height == b->Height && a->Fmt == b->Fmt && a->Fmt == ImgFmt::Lum8u);

	uint32 sumDiff = 0;
	for (int y = 0; y < a->Height; y++)
	{
		int width = a->Width;
		uint8* aline = a->RowPtr8u(y);
		uint8* bline = b->RowPtr8u(y);
		for (int x = 0; x < width; x++)
			sumDiff += AbsDiff(aline[x], bline[x]);
	}

	return (float) sumDiff / (float) (a->Width * a->Height);
}

void MotionDetector::ComputePixelsInMotion(Image* frame)
{
	if (!DebugImage && OutputDebugImage)
		DebugImage = frame->Clone();

	// minNoise and threshold are empirically tweaked
	float minNoise = 0.25f;
	float threshold = 16.0f;

	float thresholdf32 = std::max(Noise, minNoise) * threshold / 255.0f;
	int nmoving = 0;
	for (int y = 0; y < frame->Height; y++)
	{
		float* lstable = Stable->RowPtr32f(y);
		uint8* lframe = frame->RowPtr8u(y);
		for (int i = 0; i < frame->Width; i++)
		{
			float diff = fabs(lstable[i] - (lframe[i] / 255.0f));
			if (diff > thresholdf32)
			{
				nmoving++;
				if (OutputDebugImage)
					DebugImage->RowPtr8u(y)[i] = 255;
			}
			else
			{
				if (OutputDebugImage)
					DebugImage->RowPtr8u(y)[i] = 0;
			}
		}
	}

	IsMotion = nmoving >= 3;
}

void MotionDetector::MergeIntoPrev(Image* frame)
{
	if (!Stable)
		Stable = frame->Clone(ImgFmt::Lum32f);

	// Greater numbers cause a more "sticky" stable image
	// Lower numbers would allow somebody to trick the system by moving vewwy vewwy slowly
	const float stability = 100.0f;

	for (int y = 0; y < Stable->Height; y++)
	{
		float* lstable = Stable->RowPtr32f(y);
		uint8* lframe = frame->RowPtr8u(y);
		for (int x = 0; x < Stable->Width; x++)
		{
			// It's hard to know how much to make this adjustment.
			// It needs to be pretty slow to converge - otherwise you can fool the system
			// by simply moving very slowly.
			float a = stability * lstable[x];
			float b = 1.0f / 255.0f * (float) lframe[x];
			lstable[x] = (a + b) / (stability + 1.0f);
		}
	}
}

}