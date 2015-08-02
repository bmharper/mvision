#include "pch.h"
#include "win_capture/capture_device.h"
#include "Tracker.h"

static nuSysWnd*		MainWnd;
static CaptureDevice*	Camera;
static int				Entropy1;
static int				Entropy2;
static Tracker*			Track;
static nuDomEl*			TrackDiv;

static bool OnTouch( const nuEvent& ev );
static bool OnTimer( const nuEvent& ev );
static void InitializeCamera();

void nuMain( nuMainEvent ev )
{
	switch ( ev )
	{
	case nuMainEventInit:
		{
			MainWnd = nuSysWnd::CreateWithDoc();
			nuDoc* doc = MainWnd->Doc();

			//for ( int i = 0; i < 1; i++ )
			//{
			//	nuDomEl* div = doc->Root.AddChild( nuTagDiv );
			//	div->StyleParse( "width: 100px; height: 100px; border-radius: 15px; display: inline;" );
			//	div->StyleParse( "margin: 3px;" );
			//	div->StyleParse( "background: #e00e" );
			//}

			nuDomEl* big = doc->Root.AddChild( nuTagDiv );
			big->StyleParse( "width: 640px; height: 480px; display: inline;" );
			//big->StyleParse( "margin: 3px;" );
			big->StyleParse( "background: #ffff" );

			TrackDiv = big->AddChild( nuTagDiv );
			TrackDiv->StyleParse( "width: 30px; height: 30px; display: inline; position: absolute; left: 5px; top: 5px;" );
			//TrackDiv->StyleParse( "margin: 0px;" );
			TrackDiv->StyleParse( "background: #0a0a" );

			nuStyle style = big->GetStyle();
			nuStyleAttrib bgimage;
			//bgimage.SetBackgroundImage( nuImageStore::NullImageName, doc );
			bgimage.SetBackgroundImage( "NULL", doc );
			style.Set( bgimage );
			big->HackSetStyle( style );

			doc->Root.OnMouseMove( OnTouch, big );
			doc->Root.OnTouch( OnTouch, big );
			doc->Root.OnTimer( OnTimer, big );

			MainWnd->Show();

			InitializeCamera();
		}
		break;
	case nuMainEventShutdown:
		if ( Camera )
			Camera->Close();
		SafeRelease( &Camera );
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

bool OnTimer( const nuEvent& ev )
{
	nuDomEl* big = (nuDomEl*) ev.Context;
	nuDoc* doc = big->GetDoc();

	if ( Camera )
	{
		void* cameraFrame = Camera->GetNextFrame();
		if ( cameraFrame )
		{
			nuImage* img = new nuImage();
			int width = Camera->GetWidth();
			int height = Camera->GetHeight();
			uint32* buf = (uint32*) malloc( width * height * 4 );
			uint8* lineIn = (uint8*) cameraFrame;
			uint32* lineOut = buf;
			for ( int y = 0; y < height; y++ )
			{
				uint8* in = lineIn;
				for ( int x = 0; x < width; x++, in += 3 )
					lineOut[x] = NURGBA(in[2], in[1], in[0], 255);
				lineIn += width * 3;
				lineOut += width;
			}
			img->Set( width, height, buf );
			AddToTracker( width, height, cameraFrame );
			free(buf);
			free(cameraFrame);
			doc->Images.Set( "myimage", img );
		}
	}
	else
	{
		const int width = 64;
		const int height = 48;
		nuImage* img = new nuImage();
		size_t bsize = width * height * 4;
		uint32* buf = (uint32*) malloc( bsize );
		nuRGBA c;
		c.r = (uint8) (Entropy1 >> 1);
		c.g = (uint8) (Entropy2 >> 2);
		c.b = 0;
		c.a = 255;
		Entropy1++;
		Entropy2++;
		for ( int y = 0; y < height; y++ )
		{
			uint32* line = buf + y * width;
			for ( int x = 0; x < width; x++ )
			{
				c.r++;
				c.g++;
				line[x] = c.u;
			}
		}
		img->Set( width, height, buf );
		free( buf );
		doc->Images.Set( "myimage", img );
	}
	return true;
}

bool OnTouch( const nuEvent& ev )
{
	nuDomEl* big = (nuDomEl*) ev.Context;
	nuDoc* doc = big->GetDoc();

	nuStyle style = big->GetStyle();
	nuStyleAttrib bgimage;
	//bgimage.SetBackgroundImage( nuImageStore::NullImageName, doc );
	bgimage.SetBackgroundImage( "myimage", doc );

	style.Set( bgimage );
	big->HackSetStyle( style );
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
