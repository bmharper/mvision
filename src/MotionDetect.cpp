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
	delete NoiseImg;
}

void MotionDetector::Frame(Image* frame)
{
	assert(frame->Fmt == ImgFmt::Lum8u);

	const int width = 20;

	Image* myframe = frame;
	if (frame->Width > width)
		myframe = frame->HalfSize_Box_Until(false, width);

	if (Prev)
		ComputeNoise(Prev, myframe);

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

void MotionDetector::ComputeNoise(Image* a, Image* b)
{
	assert(a->Width == b->Width && a->Height == b->Height && a->Fmt == b->Fmt && a->Fmt == ImgFmt::Lum8u);

	if (!NoiseImg)
	{
		NoiseImg = new Image();
		if (!NoiseImg->Alloc(ImgFmt::Lum32f, a->Width, a->Height))
		{
			delete NoiseImg;
			return;
		}
		NoiseImg->FillBytes(0);
	}

	for (int y = 0; y < a->Height; y++)
	{
		int width = a->Width;
		uint8* aline = a->RowPtr8u(y);
		uint8* bline = b->RowPtr8u(y);
		float* nline = NoiseImg->RowPtr32f(y);
		for (int x = 0; x < width; x++)
		{
			uint32 diff = AbsDiff(aline[x], bline[x]);
			if (diff < 5)
				nline[x] = nline[x] * 0.98f + (float) diff * 0.02f;
		}
	}
}

void MotionDetector::ComputePixelsInMotion(Image* frame)
{
	if (!DebugImage && OutputDebugImage)
		DebugImage = frame->Clone();

	// constants here are empirically tweaked
	//float minNoise = 0.25f;
	//float maxNoise = 3.00f;
	//float threshold_local = 16.0f;
	//float threshold_global = 0.1f;
	//float cnoise = Clamp(Noise, minNoise, maxNoise);

	//float threshold_local_f32 = maxNoise * threshold_local / 255.0f;
	int nmoving = 0;
	float totalDiff = 0;
	float totalNoise = 0;
	for (int y = 0; y < frame->Height; y++)
	{
		float* lstable = Stable->RowPtr32f(y);
		uint8* lframe = frame->RowPtr8u(y);
		float* lnoise = NoiseImg->RowPtr32f(y);
		for (int i = 0; i < frame->Width; i++)
		{
			float diff = fabs(lstable[i] - (lframe[i] * (1.0f / 255.0f)));
			totalDiff += diff;
			totalNoise += lnoise[i];
			float threshold_local = lnoise[i] * 0.5;
			if (diff > threshold_local)
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
	Noise = totalNoise;
	GlobalAvgDiff = totalDiff / (frame->Width * frame->Height);

	IsLocalMotion = nmoving >= 3;
	IsGlobalMotion = GlobalAvgDiff > totalNoise * 0.001;
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