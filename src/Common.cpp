#include "pch.h"
#include "Common.h"

namespace sx
{

// Use 'div.exe' by AMD to calculate these things. Valid ranges are up to 32k or 64k, I'm not sure.
inline uint32 DivideByThree(uint32 i)
{
	i *= 0xAAAB;
	return i >> 17;
}

void Util_ImageToCanvas(const Image* img, xoCanvas2D* ccx)
{
	assert(img->Fmt == ImgFmt::RGB8u);

	for (int y = 0; y < img->Height; y++)
	{
		uint8* lineIn = (uint8*) img->RowPtr(y);
		uint32* lineOut = (uint32*) ccx->RowPtr(y);
		uint8* in = lineIn;
		int width = img->Width;
		for (int x = 0; x < width; x++, in += 3)
			lineOut[x] = xoRGBA::RGBA(in[2], in[1], in[0], 255).u;
	}
	ccx->Invalidate();
}

template<int scale>
void WriteLumPxToCanvas(uint32* out, uint8 v)
{
	uint32 px = xoRGBA::RGBA(v, v, v, 255).u;
	out[0] = px;
	if (scale >= 2)
	{
		out[1] = px;
	}
	if (scale == 4)
	{
		out[2] = px;
		out[3] = px;
	}
}

void Util_LumToCanvas(const Image* lum, xoCanvas2D* ccx, int canvasX, int canvasY, int scale)
{
	assert(canvasX + lum->Width * scale <= ccx->Width());
	assert(canvasY + lum->Height * scale <= ccx->Height());
	int outY = 0;
	for (int y = 0; y < lum->Height; y++, outY += scale)
	{
		for (int lineRepeat = 0; lineRepeat < scale; lineRepeat++)
		{
			uint32* out = ((uint32*) ccx->RowPtr(outY + canvasY + lineRepeat)) + canvasX;
			if (lum->Fmt == ImgFmt::Lum8u)
			{
				uint8* in = (uint8*) lum->RowPtr(y);
				for (int x = 0; x < lum->Width; x++, out += scale)
				{
					if (scale == 1)			WriteLumPxToCanvas<1>(out, in[x]);
					else if (scale == 2)	WriteLumPxToCanvas<2>(out, in[x]);
					else if (scale == 4)	WriteLumPxToCanvas<4>(out, in[x]);
				}
			}
			else if (lum->Fmt == ImgFmt::Lum32f)
			{
				float* in = (float*) lum->RowPtr(y);
				for (int x = 0; x < lum->Width; x++, out += scale)
				{
					if (scale == 1)			WriteLumPxToCanvas<1>(out, (byte) (in[x] * 255.0f));
					else if (scale == 2)	WriteLumPxToCanvas<2>(out, (byte) (in[x] * 255.0f));
					else if (scale == 4)	WriteLumPxToCanvas<4>(out, (byte) (in[x] * 255.0f));
				}
			}
		}
	}
	ccx->Invalidate();
}

void Util_SetupTestUI(xoDoc* doc, std::function<void(Image* frame, xoCanvas2D* cx, xoDomNode* label)> onTimer)
{
	xoDomCanvas* canvas1 = doc->Root.AddCanvas();
	xoDomNode* label = doc->Root.AddNode(xoTagDiv);

	auto myOnTimer = [canvas1, label, onTimer](const xoEvent& ev)
	{
		auto cam = Global.Camera;
		if (cam)
		{
			Image* frame = cam->NextFrame();
			if (frame)
			{
				xoCanvas2D* cx1 = canvas1->GetCanvas2D();
				Util_ImageToCanvas(frame, cx1);

				onTimer(frame, cx1, label);

				canvas1->ReleaseCanvas(cx1);
				free(frame);
			}
		}
		return true;
	};

	canvas1->SetSize(640, 480);
	canvas1->StyleParse("background: #ffaf");
	doc->Root.OnTimer(myOnTimer, 5);
}

}