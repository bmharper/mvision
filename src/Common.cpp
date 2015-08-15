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

void Util_LumToCanvas(ccv_dense_matrix_t* lum, xoCanvas2D* ccx, int canvasX, int canvasY, int scale)
{
	assert(canvasX + lum->cols * scale <= ccx->Width());
	assert(canvasY + lum->rows * scale <= ccx->Height());
	int outY = 0;
	for (int y = 0; y < lum->rows; y++, outY += scale)
	{
		for (int lineRepeat = 0; lineRepeat < scale; lineRepeat++)
		{
			uint32* out = ((uint32*) ccx->RowPtr(outY + canvasY + lineRepeat)) + canvasX;
			if (CCV_GET_DATA_TYPE(lum->type) == CCV_8U)
			{
				uint8* in = lum->data.u8 + y * lum->step;
				for (int x = 0; x < lum->cols; x++, out += scale)
				{
					if (scale == 1)			WriteLumPxToCanvas<1>(out, in[x]);
					else if (scale == 2)	WriteLumPxToCanvas<2>(out, in[x]);
					else if (scale == 4)	WriteLumPxToCanvas<4>(out, in[x]);
				}
			}
			else if (CCV_GET_DATA_TYPE(lum->type) == CCV_32F)
			{
				float* in = (float*) (lum->data.u8 + y * lum->step);
				for (int x = 0; x < lum->cols; x++, out += scale)
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
ccv_dense_matrix_t* Util_Lum_HalfSize_Box_T(ccv_dense_matrix_t* lum)
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

ccv_dense_matrix_t* Util_Lum_HalfSize_Box(ccv_dense_matrix_t* lum, bool sRGB)
{
	if (sRGB)
		return Util_Lum_HalfSize_Box_T<true>(lum);
	else
		return Util_Lum_HalfSize_Box_T<false>(lum);
}

ccv_dense_matrix_t* Util_Lum_HalfSize_Box_Until(ccv_dense_matrix_t* lum, bool sRGB, int widthLessThanOrEqualTo)
{
	assert(lum->cols > widthLessThanOrEqualTo);

	ccv_dense_matrix_t* image = lum;
	while (image->cols > widthLessThanOrEqualTo)
	{
		ccv_dense_matrix_t* next = Util_Lum_HalfSize_Box(image, sRGB);
		if (image != lum)
			ccv_matrix_free(image);
		image = next;
	}
	return image;
}

ccv_dense_matrix_t* Util_Clone(ccv_dense_matrix_t* org, int targetType)
{
	if (targetType == 0 || targetType == CCV_GET_DATA_TYPE(org->type))
	{
		ccv_dense_matrix_t* copy = ccv_dense_matrix_new(org->rows, org->cols, org->type, nullptr, org->sig);
		size_t dataSize = ccv_compute_dense_matrix_size(org->rows, org->cols, org->type) - sizeof(ccv_dense_matrix_t);
		memcpy(copy->data.u8, org->data.u8, dataSize);
		return copy;
	}
	else
	{
		ccv_dense_matrix_t* copy = ccv_dense_matrix_new(org->rows, org->cols, targetType | CCV_GET_CHANNEL(org->type), nullptr, 0);
		int npixels = org->rows * org->cols;
		if (CCV_GET_DATA_TYPE(org->type) == CCV_8U && targetType == CCV_32F)
		{
			for (int i = 0; i < npixels; i++)
				copy->data.f32[i] = (float) org->data.u8[i] / 255.0f;
		}
		else
		{
			assert(false);
		}
		return copy;
	}
}

void Util_Copy(ccv_dense_matrix_t* dst, ccv_dense_matrix_t* src)
{
	assert(CCV_GET_CHANNEL(src->type) == CCV_GET_CHANNEL(dst->type));
	assert(CCV_GET_DATA_TYPE(src->type) == CCV_GET_DATA_TYPE(dst->type));
	assert(src->rows == dst->rows);
	assert(src->cols == dst->cols);

	size_t dataSize = ccv_compute_dense_matrix_size(src->rows, src->cols, src->type) - sizeof(ccv_dense_matrix_t);
	memcpy(dst->data.u8, src->data.u8, dataSize);
}
