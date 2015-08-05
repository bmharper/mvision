#include "pch.h"
#include "Common.h"

// Use 'div.exe' by AMD to calculate these things. Valid ranges are up to 32k or 64k, I'm not sure.
inline uint32 DivideByThree(uint32 i)
{
	i *= 0xAAAB;
	return i >> 17;
}

void Util_CameraToCanvas(CaptureDevice* camera, const void* cameraFrame, xoCanvas2D* ccx)
{
	// Assume camera is RGB. Canvas is BGRA
	int width = camera->GetWidth();
	int height = camera->GetHeight();
	
	uint8* lineIn = (uint8*) cameraFrame;
	for (int y = 0; y < height; y++)
	{
		uint32* lineOut = (uint32*) ccx->RowPtr(y);
		uint8* in = lineIn;
		for (int x = 0; x < width; x++, in += 3)
			lineOut[x] = xoRGBA::RGBA(in[2], in[1], in[0], 255).u;
		lineIn += width * 3;
	}
	ccx->Invalidate();
}

void Util_LumToCanvas(ccv_dense_matrix_t* lum, xoCanvas2D* ccx)
{
	for (int y = 0; y < lum->rows; y++)
	{
		uint32* lineOut = (uint32*) ccx->RowPtr(y);
		uint8* in = lum->data.u8 + y * lum->step;
		for (int x = 0; x < lum->cols; x++, in++)
			lineOut[x] = xoRGBA::RGBA(*in, *in, *in, 255).u;
	}
	ccx->Invalidate();
}

ccv_dense_matrix_t* Util_RGB_to_CCV_Lum8(int width, int height, const void* rgb)
{
	ccv_dense_matrix_t* lum = ccv_dense_matrix_new(height, width, CCV_8U | CCV_C1, nullptr, 0);
	Util_RGB_to_Lum(width, height, rgb, lum);
	return lum;
}

void Util_RGB_to_Lum(int width, int height, const void* rgb, ccv_dense_matrix_t* lum)
{
	for (int y = 0; y < height; y++)
	{
		uint8* srcLine = ((uint8*) rgb) + y * width * 3;
		uint8* dstLine = lum->data.u8 + y * lum->step;
		for (int x = 0, x3 = 0; x < width; x++, x3 += 3)
			dstLine[x] = DivideByThree(srcLine[x3] + srcLine[x3 + 1] + srcLine[x3 + 2]);
	}
}

template<bool sRGB>
ccv_dense_matrix_t* Util_Lum_HalfSize_Cheap_T(ccv_dense_matrix_t* lum)
{
	assert((lum->cols & 1) == 0 && (lum->rows & 1) == 0);

	ccv_dense_matrix_t* half = ccv_dense_matrix_new(lum->rows / 2, lum->cols / 2, CCV_8U | CCV_C1, nullptr, 0);

	int nwidth = lum->cols / 2;
	int nheight = lum->rows / 2;
	for (int y = 0; y < nheight; y++)
	{
		uint8* srcLine1 = lum->data.u8 + (y << 1) * lum->step;
		uint8* srcLine2 = lum->data.u8 + ((y << 1) + 1) * lum->step;
		uint8* dstLine = half->data.u8 + y * half->step;
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

ccv_dense_matrix_t* Util_Lum_HalfSize_Cheap(ccv_dense_matrix_t* lum)
{
	return Util_Lum_HalfSize_Cheap_T<false>(lum);
}

ccv_dense_matrix_t* Util_Lum_HalfSize_Cheap_Linear(ccv_dense_matrix_t* lum)
{
	return Util_Lum_HalfSize_Cheap_T<true>(lum);
}

