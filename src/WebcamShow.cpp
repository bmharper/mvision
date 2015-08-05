#include "pch.h"
#include "win_capture/capture_device.h"
#include "Tracker.h"

static xoSysWnd*		MainWnd;
static CaptureDevice*	Camera;
static int				Entropy1;
static int				Entropy2;
static Tracker*			Track;
static xoInternalID		CanvasID;

struct UIState_t
{
	bool		IsDragging = false;
	bool		StartTrack = false;
	ccv_rect_t	TrackBox;
} UIState;

static bool OnMouseMove(const xoEvent& ev);
static bool OnMouseDown(const xoEvent& ev);
static bool OnMouseUp(const xoEvent& ev);
static bool OnTimer(const xoEvent& ev);
static void InitializeCamera();

void xoMain(xoMainEvent ev)
{
	switch (ev)
	{
	case xoMainEventInit:
	{
		MainWnd = xoSysWnd::CreateWithDoc();
		xoDoc* doc = MainWnd->Doc();

		//xoDomNode* big = doc->Root.AddNode(xoTagDiv);
		//big->StyleParse("width: 640px; height: 480px;");
		//big->StyleParse("background: #ffaf");

		xoDomCanvas* canvas = doc->Root.AddCanvas();
		//canvas->StyleParse("width: 640px; height: 480px;");
		canvas->SetSize(640, 480);
		canvas->StyleParse("background: #ffaf");
		CanvasID = canvas->GetInternalID();

		//xoStyle style = big->GetStyle();
		//xoStyleAttrib bgimage;
		//bgimage.SetBackgroundImage("NULL", doc);
		//style.Set(bgimage);
		//big->HackSetStyle(style);

		doc->Root.OnMouseMove([doc](const xoEvent& ev) -> bool {
			UIState.TrackBox.width = (int) (ev.Points[0].x - UIState.TrackBox.x);
			UIState.TrackBox.height = (int) (ev.Points[0].y - UIState.TrackBox.y);
			return true;
		});
		doc->Root.OnMouseDown([](const xoEvent& ev) -> bool {
			delete Track;
			Track = nullptr;
			UIState.TrackBox.x = (int) ev.Points[0].x;
			UIState.TrackBox.y = (int) ev.Points[0].y;
			UIState.IsDragging = true;
			return true;
		});
		doc->Root.OnMouseUp([](const xoEvent& ev) -> bool {
			UIState.TrackBox.width = (int) (ev.Points[0].x - UIState.TrackBox.x);
			UIState.TrackBox.height = (int) (ev.Points[0].y - UIState.TrackBox.y);
			UIState.IsDragging = false;
			UIState.StartTrack = true;
			return true;
		});
		doc->Root.OnTimer(OnTimer, canvas, 15);

		MainWnd->Show();

		InitializeCamera();
	}
	break;
	case xoMainEventShutdown:
		if (Camera)
			Camera->Close();
		SafeRelease(&Camera);
		MFShutdown();
		CoUninitialize();
		delete Track;
		Track = NULL;
		delete MainWnd;
		MainWnd = NULL;
		break;
	}
}

static void StartTracker(int width, int height, void* rgb24, ccv_rect_t box)
{
	delete Track;
	Track = new Tracker();
	Track->Initialize(width, height, rgb24, box);
}

static ccv_rect_t AddToTracker(int width, int height, void* rgb24)
{
	ccv_comp_t box;
	ccv_tld_info_t info;
	Track->Track(rgb24, box, info);
	return box.rect;
}

bool OnTimer(const xoEvent& ev)
{
	//xoDomEl* big = (xoDomEl*) ev.Context;
	xoDomCanvas* canvas = (xoDomCanvas*) ev.Context;
	xoDoc* doc = canvas->GetDoc();

	if (Camera)
	{
		void* cameraFrame = Camera->GetNextFrame();
		if (cameraFrame)
		{
			//xoImage* img = new xoImage();
			int width = Camera->GetWidth();
			int height = Camera->GetHeight();
			//uint32* buf = (uint32*) malloc(width * height * 4);
			auto c2d = canvas->GetCanvas2D();

			uint8* lineIn = (uint8*) cameraFrame;
			//uint32* lineOut = buf;
			for (int y = 0; y < height; y++)
			{
				uint32* lineOut = (uint32*) c2d->RowPtr(y);
				uint8* in = lineIn;
				for (int x = 0; x < width; x++, in += 3)
					lineOut[x] = xoRGBA::RGBA(in[2], in[1], in[0], 255).u;
				lineIn += width * 3;
				//lineOut += width;
			}
			//img->Set(xoTexFormatRGBA8, width, height, buf);
			auto *tbox = &UIState.TrackBox;
			if (UIState.StartTrack)
			{
				UIState.StartTrack = false;
				StartTracker(width, height, cameraFrame, *tbox);
			}
			else if (Track)
				*tbox = AddToTracker(width, height, cameraFrame);
			//free(buf);
			c2d->Invalidate();
			if (UIState.IsDragging || Track)
				c2d->StrokeRect(xoBox(tbox->x, tbox->y, tbox->x + tbox->width, tbox->y + tbox->height), xoColor::RGBA(200, 0, 0, 200), 1);
			canvas->ReleaseCanvas(c2d);
			free(cameraFrame);
			//doc->Images.Set("myimage", img);
		}
	}
	return true;
}

void InitializeCamera()
{
	HRESULT hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );
	Camera = new CaptureDevice();
	std::string error;
	if ( !Camera->InitializeFirst( error ) )
	{
		delete Camera;
		Camera = NULL;
	}
}
