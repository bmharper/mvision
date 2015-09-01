#include "pch.h"
#include "Common.h"
#include "Tracker.h"

#ifdef SX_CCV

static struct TLD_t
{
	bool		IsDragging = false;
	bool		StartTrack = false;
	ccv_rect_t	TrackBox;
	Tracker*	Track;
} TLD;

static void StartTracker(int width, int height, void* rgb24, ccv_rect_t box)
{
	delete TLD.Track;
	TLD.Track = new Tracker();
	TLD.Track->Initialize(width, height, rgb24, box);
}

static ccv_rect_t AddToTracker(int width, int height, void* rgb24)
{
	ccv_comp_t box;
	ccv_tld_info_t info;
	TLD.Track->Track(rgb24, box, info);
	return box.rect;
}

static bool OnTimer(const xoEvent& ev)
{
	xoDomCanvas* canvas = (xoDomCanvas*) ev.Context;

	if (Global.Camera)
	{
		void* cameraFrame = Global.Camera->NextFrame();
		if (cameraFrame)
		{
			int width = Global.Camera->Width();
			int height = Global.Camera->Height();
			auto c2d = canvas->GetCanvas2D();

			Util_CameraToCanvas(Global.Camera, cameraFrame, c2d);

			auto *tbox = &TLD.TrackBox;
			if (TLD.StartTrack)
			{
				TLD.StartTrack = false;
				StartTracker(width, height, cameraFrame, *tbox);
			}
			else if (TLD.Track)
				*tbox = AddToTracker(width, height, cameraFrame);
			if (TLD.IsDragging || TLD.Track)
				c2d->StrokeRect(xoBox(tbox->x, tbox->y, tbox->x + tbox->width, tbox->y + tbox->height), xoColor::RGBA(200, 0, 0, 200), 1);

			canvas->ReleaseCanvas(c2d);
			free(cameraFrame);
		}
	}
	return true;
}

void TLD_Start(xoDoc* doc)
{
	xoDomCanvas* canvas = doc->Root.AddCanvas();
	canvas->SetSize(640, 480);
	canvas->StyleParse("background: #ffaf");

	doc->Root.OnMouseMove([doc](const xoEvent& ev) -> bool {
		TLD.TrackBox.width = (int) (ev.Points[0].x - TLD.TrackBox.x);
		TLD.TrackBox.height = (int) (ev.Points[0].y - TLD.TrackBox.y);
		return true;
	});
	doc->Root.OnMouseDown([](const xoEvent& ev) -> bool {
		delete TLD.Track;
		TLD.Track = nullptr;
		TLD.TrackBox.x = (int) ev.Points[0].x;
		TLD.TrackBox.y = (int) ev.Points[0].y;
		TLD.IsDragging = true;
		return true;
	});
	doc->Root.OnMouseUp([](const xoEvent& ev) -> bool {
		TLD.TrackBox.width = (int) (ev.Points[0].x - TLD.TrackBox.x);
		TLD.TrackBox.height = (int) (ev.Points[0].y - TLD.TrackBox.y);
		TLD.IsDragging = false;
		TLD.StartTrack = true;
		return true;
	});
	doc->Root.OnTimer(OnTimer, canvas, 15);
}

void TLD_End()
{
	delete TLD.Track;
	TLD.Track = NULL;
}

#endif