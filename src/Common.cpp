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

template<bool sRGB>
Image* Util_Lum_HalfSize_Box_T(Image* lum)
{
	assert((lum->Width & 1) == 0 && (lum->Height & 1) == 0);

	Image* half = new Image();
	if (!half->Alloc(lum->Fmt, lum->Width / 2, lum->Height / 2))
	{
		delete half;
		return nullptr;
	}

	int nwidth = lum->Width / 2;
	int nheight = lum->Height / 2;
	for (int y = 0; y < nheight; y++)
	{
		uint8* srcLine1 = (uint8*) lum->RowPtr(y * 2);
		uint8* srcLine2 = (uint8*) lum->RowPtr(y * 2 + 1);
		uint8* dstLine = (uint8*) half->RowPtr(y);
		int x2 = 0;
		for (int x = 0; x < nwidth; x++, x2 += 2)
		{
			if (sRGB)
			{
				float sum1 = xoSRGB2Linear(srcLine1[x2]) + xoSRGB2Linear(srcLine1[x2 + 1]);
				float sum2 = xoSRGB2Linear(srcLine2[x2]) + xoSRGB2Linear(srcLine2[x2 + 1]);
				dstLine[x] = xoLinear2SRGB((sum1 + sum2) / 4.0f);
			}
			else
			{
				uint32 sum1 = (uint32) srcLine1[x2] + (uint32) srcLine1[x2 + 1];
				uint32 sum2 = (uint32) srcLine2[x2] + (uint32) srcLine2[x2 + 1];
				dstLine[x] = (sum1 + sum2) / 4;
			}
		}
	}
	return half;
}

Image* Util_Lum_HalfSize_Box(Image* lum, bool sRGB)
{
	if (sRGB)
		return Util_Lum_HalfSize_Box_T<true>(lum);
	else
		return Util_Lum_HalfSize_Box_T<false>(lum);
}

Image* Util_Lum_HalfSize_Box_Until(Image* lum, bool sRGB, int widthLessThanOrEqualTo)
{
	assert(lum->Width > widthLessThanOrEqualTo);

	Image* image = lum;
	while (image->Width > widthLessThanOrEqualTo)
	{
		Image* next = Util_Lum_HalfSize_Box(image, sRGB);
		if (image != lum)
			delete image;
		image = next;
	}
	return image;
}

/*
void Util_RGB_to_Lum(int width, int height, const void* rgb, Image* lum)
{
	for (int y = 0; y < height; y++)
	{
		uint8* srcLine = ((uint8*) rgb) + y * width * 3;
		uint8* dstLine = lum->data.u8 + y * lum->step;
		for (int x = 0, x3 = 0; x < width; x++, x3 += 3)
			dstLine[x] = DivideByThree(srcLine[x3] + srcLine[x3 + 1] + srcLine[x3 + 2]);
	}
}
*/

/*
ccv_dense_matrix_t* Util_RGB_to_CCV_Lum8(int width, int height, const void* rgb)
{
	ccv_dense_matrix_t* lum = ccv_dense_matrix_new(height, width, CCV_8U | CCV_C1, nullptr, 0);
	Util_RGB_to_Lum(width, height, rgb, lum);
	return lum;
}
*/

}