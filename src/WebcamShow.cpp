#include "pch.h"
#include "win_capture/capture_device.h"
#include "Tracker.h"

static xoSysWnd*		MainWnd;
static CaptureDevice*	Camera;
static int				Entropy1;
static int				Entropy2;
static Tracker*			Track;
static xoDomNode*		TrackDiv;
static xoInternalID		CanvasID;

static bool OnTouch( const xoEvent& ev );
static bool OnTimer( const xoEvent& ev );
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

		//TrackDiv = big->AddNode(xoTagDiv);
		//TrackDiv->StyleParse("width: 30px; height: 30px; display: inline; position: absolute; left: 5px; top: 5px;");
		//TrackDiv->StyleParse("background: #0a0a");

		//xoStyle style = big->GetStyle();
		//xoStyleAttrib bgimage;
		//bgimage.SetBackgroundImage("NULL", doc);
		//style.Set(bgimage);
		//big->HackSetStyle(style);

		doc->Root.OnMouseMove(OnTouch, canvas);
		doc->Root.OnTouch(OnTouch, canvas);
		doc->Root.OnTimer(OnTimer, canvas);

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

static void AddToTracker( int width, int height, void* rgb24 )
{
	ccv_rect_t box = ccv_rect(160, 200, 160, 200);
	if ( !Track )
	{
		Track = new Tracker();
		Track->Initialize( width, height, rgb24, box );
	}
	else
	{
		ccv_comp_t newbox;
		ccv_tld_info_t info;
		Track->Track( rgb24, newbox, info );
		box = newbox.rect;
	}
	TrackDiv->StyleParsef( "left: %dpx; top: %dpx; width: %dpx; height: %dpx;", box.x, box.y, box.width, box.height );
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
			//AddToTracker(width, height, cameraFrame);
			//free(buf);
			c2d->Invalidate();
			canvas->ReleaseCanvas(c2d);
			free(cameraFrame);
			//doc->Images.Set("myimage", img);
		}
	}
	else
	{
		const int width = 64;
		const int height = 48;
		xoImage* img = new xoImage();
		size_t bsize = width * height * 4;
		uint32* buf = (uint32*) malloc(bsize);
		xoRGBA c;
		c.r = (uint8) (Entropy1 >> 1);
		c.g = (uint8) (Entropy2 >> 2);
		c.b = 0;
		c.a = 255;
		Entropy1++;
		Entropy2++;
		for (int y = 0; y < height; y++)
		{
			uint32* line = buf + y * width;
			for (int x = 0; x < width; x++)
			{
				c.r++;
				c.g++;
				line[x] = c.u;
			}
		}
		img->Set(xoTexFormatRGBA8, width, height, buf);
		free(buf);
		doc->Images.Set("myimage", img);
	}
	return true;
}

bool OnTouch(const xoEvent& ev)
{
	OnTimer(ev);

	/*
	xoDomNode* big = (xoDomNode*) ev.Context;
	xoDoc* doc = big->GetDoc();
	
	xoStyle style = big->GetStyle();
	xoStyleAttrib bgimage;
	//bgimage.SetBackgroundImage( xoImageStore::NullImageName, doc );
	bgimage.SetBackgroundImage("myimage", doc);
	
	style.Set(bgimage);
	big->HackSetStyle(style);
	*/
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
