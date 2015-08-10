#include "pch.h"
#include "Common.h"

class MotionDetector
{
public:
	ccv_dense_matrix_t*		DebugImage = nullptr;
	ccv_dense_matrix_t*		Prev = nullptr;			// Our previous frame
	ccv_dense_matrix_t*		Stable = nullptr;		// Our long-term idea of what the scene is supposed to look like
	float					Noise = 0;
	int64_t					NFrames = 0;
	bool					IsMotion = false;

	MotionDetector();
	~MotionDetector();
	void					Frame(ccv_dense_matrix_t* frame);

private:
	float					ComputeNoise(ccv_dense_matrix_t* a, ccv_dense_matrix_t* b);
	void					ComputePixelsInMotion(ccv_dense_matrix_t* frame);
	void					MergeIntoPrev(ccv_dense_matrix_t* frame);

	inline static uint32	AbsDiff(uint8 a, uint8 b) { int32 d = (int32) a - (int32) b; return (uint32) (d < 0 ? -d : d); }

};

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
	
	const int width = 40;

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
	if (!DebugImage)
		DebugImage = Util_Clone(frame);
	
	// empirically tweaked
	float minNoise = 1.0f;

	// also empirically tweaked
	float threshold = 6.0f;

	float thresholdf32 = std::max(Noise, minNoise) * threshold / 255.0f;
	int npixels = Stable->cols * Stable->rows;
	int nmoving = 0;
	for (int i = 0; i < npixels; i++)
	{
		float diff = fabs(Stable->data.f32[i] - (frame->data.u8[i] / 255.0f));
		if (diff > thresholdf32)
		{
			nmoving++;
			DebugImage->data.u8[i] = 255;
		}
		else
		{
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
	const float stability = 150.0f;

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

static struct Motion_t
{
	xoDomCanvas*	Canvas1;
	xoDomCanvas*	Canvas2;
	xoDomCanvas*	Canvas3;
	xoDomCanvas*	Canvas4;
	xoDomNode*		Stats;
	MotionDetector*	Detector;
} Motion;

static void UpdateStats()
{
	auto msg = fmt("Noise: %.3f. %s", Motion.Detector->Noise, Motion.Detector->IsMotion ? "MOVE" : "--");
	Motion.Stats->SetText(msg.Z);
}

static bool OnTimer(const xoEvent& ev)
{
	auto cam = Global.Camera;
	if (cam)
	{
		void* cameraFrame = cam->GetNextFrame();
		if (cameraFrame)
		{
			auto c1 = Motion.Canvas1->GetCanvas2D();
			Util_CameraToCanvas(cam, cameraFrame, c1);
			Motion.Canvas1->ReleaseCanvas(c1);

			auto c2 = Motion.Canvas2->GetCanvas2D();
			auto c3 = Motion.Canvas3->GetCanvas2D();
			auto c4 = Motion.Canvas4->GetCanvas2D();
			ccv_dense_matrix_t* lum = Util_RGB_to_CCV_Lum8(cam->GetWidth(), cam->GetHeight(), cameraFrame);
			ccv_dense_matrix_t* half = Util_Lum_HalfSize_Box(lum, false);
			ccv_dense_matrix_t* quarter = Util_Lum_HalfSize_Box(half, false);
			Motion.Detector->Frame(quarter);
			Util_LumToCanvas(half, c2);
			Util_LumToCanvas(quarter, c3);
			if (Motion.Detector->Stable)
				Util_LumToCanvas(Motion.Detector->Stable, c4);
			if (Motion.Detector->DebugImage)
				Util_LumToCanvas(Motion.Detector->DebugImage, c4, 40);
			ccv_matrix_free(lum);
			ccv_matrix_free(half);
			ccv_matrix_free(quarter);
			Motion.Canvas2->ReleaseCanvas(c2);
			Motion.Canvas3->ReleaseCanvas(c3);
			Motion.Canvas4->ReleaseCanvas(c4);

			free(cameraFrame);

			UpdateStats();
		}
	}
	return true;
}

void Motion_Start(xoDoc* doc)
{
	Motion.Detector = new MotionDetector();
	xoDomCanvas* c1 = doc->Root.AddCanvas();
	xoDomCanvas* c2 = doc->Root.AddCanvas();
	xoDomCanvas* c3 = doc->Root.AddCanvas();
	xoDomCanvas* c4 = doc->Root.AddCanvas();
	xoDomNode* stats = doc->Root.AddNode(xoTagDiv);
	Motion.Canvas1 = c1;
	Motion.Canvas2 = c2;
	Motion.Canvas3 = c3;
	Motion.Canvas4 = c4;
	Motion.Stats = stats;
	c1->SetSize(640, 480);
	c1->StyleParse("background: #ffaf");
	c2->SetSize(320, 240);
	c2->StyleParse("background: #afff");
	c3->SetSize(160, 120);
	c3->StyleParse("background: #faff");
	c4->SetSize(160, 120);
	c4->StyleParse("background: #faff");
	stats->StyleParse("break: before");
	doc->Root.OnTimer(OnTimer, nullptr, 15);
}

void Motion_End()
{
	delete Motion.Detector;
}


