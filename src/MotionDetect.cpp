#include "pch.h"
#include "MotionDetect.h"
#include "Common.h"

MotionDetector::MotionDetector()
{
}

MotionDetector::~MotionDetector()
{
	if (Prev)
		ccv_matrix_free(Prev);
	if (Stable)
		ccv_matrix_free(Stable);
	if (DebugImage)
		ccv_matrix_free(DebugImage);
}

void MotionDetector::Frame(ccv_dense_matrix_t* frame)
{
	assert(CCV_GET_DATA_TYPE(frame->type) == CCV_8U);
	assert(CCV_GET_CHANNEL(frame->type) == CCV_C1);

	const int width = 20;

	ccv_dense_matrix_t* myframe = frame;
	if (frame->cols > width)
		myframe = Util_Lum_HalfSize_Box_Until(frame, false, width);

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
		Prev = Util_Clone(myframe);
	else
		Util_Copy(Prev, myframe);

	if (myframe != frame)
		ccv_matrix_free(myframe);

	NFrames++;
}

float MotionDetector::ComputeNoise(ccv_dense_matrix_t* a, ccv_dense_matrix_t* b)
{
	// uber-simple: just treat the entire scene as noise.

	uint32 sumDiff = 0;
	int npixels = a->cols * a->rows;
	for (int i = 0; i < npixels; i++)
		sumDiff += AbsDiff(a->data.u8[i], b->data.u8[i]);

	return (float) sumDiff / (float) npixels;
}

void MotionDetector::ComputePixelsInMotion(ccv_dense_matrix_t* frame)
{
	if (!DebugImage && OutputDebugImage)
		DebugImage = Util_Clone(frame);

	// minNoise and threshold are empirically tweaked
	float minNoise  =  0.25f;
	float threshold = 16.0f;

	float thresholdf32 = std::max(Noise, minNoise) * threshold / 255.0f;
	int npixels = Stable->cols * Stable->rows;
	int nmoving = 0;
	for (int i = 0; i < npixels; i++)
	{
		float diff = fabs(Stable->data.f32[i] - (frame->data.u8[i] / 255.0f));
		if (diff > thresholdf32)
		{
			nmoving++;
			if (OutputDebugImage)
				DebugImage->data.u8[i] = 255;
		}
		else
		{
			if (OutputDebugImage)
				DebugImage->data.u8[i] = 0;
		}
	}

	IsMotion = nmoving >= 3;
}

void MotionDetector::MergeIntoPrev(ccv_dense_matrix_t* frame)
{
	if (!Stable)
		Stable = Util_Clone(frame, CCV_32F);

	// Greater numbers cause a more "sticky" stable image
	// Lower numbers would allow somebody to trick the system by moving vewwy vewwy slowly
	const float stability = 100.0f;

	int npixels = frame->cols * frame->rows;
	for (int i = 0; i < npixels; i++)
	{
		// It's hard to know how much to make this adjustment.
		// It needs to be pretty slow to converge - otherwise you can fool the system
		// by simply moving very slowly.
		float a = stability * (float) Stable->data.f32[i];
		float b = 1.0f / 255.0f * (float) frame->data.u8[i];
		Stable->data.f32[i] = (a + b) / (stability + 1.0f);
	}
}
