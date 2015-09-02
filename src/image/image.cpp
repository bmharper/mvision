#include "pch.h"
#include "image.h"

namespace sx
{

// Use 'div.exe' by AMD to calculate these things. Valid ranges are up to 32k or 64k, I'm not sure.
inline uint32 DivideByThree(uint32 i)
{
	i *= 0xAAAB;
	return i >> 17;
}

static Image NullImage;

Image::~Image()
{
	Free();
}

void Image::Free()
{
	free(Buf);
	memcpy(this, &NullImage, sizeof(*this));
}

bool Image::Alloc(ImgFmt fmt, int width, int height)
{
	Free();

	int _stride = ImgStride(ImgFmt_BytesPP(fmt) * width);
	void* _buf = malloc(_stride * height);
	if (!_buf)
		return false;

	Stride = _stride;
	Buf = _buf;
	Scan0 = _buf;
	Fmt = fmt;
	Width = width;
	Height = height;
	return true;
}

void Image::CopyTo(Image* dst) const
{
	assert(dst->Width == Width);
	assert(dst->Height == Height);
	assert(dst->Fmt == Fmt);

	int lineLen = ImgFmt_BytesPP(Fmt) * Width;
	for (int y = 0; y < Height; y++)
		memcpy(dst->RowPtr(y), RowPtr(y), lineLen);
}

Image* Image::Clone(ImgFmt dstFormat) const
{
	if (dstFormat == ImgFmt::Null)
		dstFormat = Fmt;

	Image* copy = new Image();
	if (!copy->Alloc(dstFormat, Width, Height))
	{
		delete copy;
		return nullptr;
	}

	if (dstFormat == Fmt)
	{
		CopyTo(copy);
	}
	else
	{
		if (Fmt == ImgFmt::Lum8u && dstFormat == ImgFmt::Lum32f)
		{
			for (int y = 0; y < Height; y++)
			{
				uint8* src = RowPtr8u(y);
				float* dst = copy->RowPtr32f(y);
				for (int x = 0; x < Width; x++)
					dst[x] = (float) src[x] / 255.0f;
			}
		}
		else if (Fmt == ImgFmt::RGB8u && dstFormat == ImgFmt::Lum8u)
		{
			for (int y = 0; y < Height; y++)
			{
				uint8* src = RowPtr8u(y);
				uint8* dst = copy->RowPtr8u(y);
				for (int x = 0, x3 = 0; x < Width; x++, x3 += 3)
					dst[x] = DivideByThree(src[x3] + src[x3 + 1] + src[x3 + 2]);
			}
		}
		else
		{
			assert(false);
		}
	}
	return copy;
}

template<bool sRGB>
Image* Util_Lum_HalfSize_Box_T(Image* lum)
{
	assert((lum->Width & 1) == 0 && (lum->Height & 1) == 0);
	assert(lum->Fmt == ImgFmt::Lum8u);

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

Image* Image::HalfSize_Box(bool sRGB)
{
	if (sRGB)
		return Util_Lum_HalfSize_Box_T<true>(this);
	else
		return Util_Lum_HalfSize_Box_T<false>(this);
}

Image* Image::HalfSize_Box_Until(bool sRGB, int widthLessThanOrEqualTo)
{
	assert(Width > widthLessThanOrEqualTo);

	Image* image = this;
	while (image->Width > widthLessThanOrEqualTo)
	{
		Image* next = image->HalfSize_Box(sRGB);
		if (!next)
			return nullptr;
		if (image != this)
			delete image;
		image = next;
	}
	return image;
}

#ifdef SX_CCV
ccv_dense_matrix_t*	Image::ToCCV() const
{
	assert(ImgFmt_NChan(Fmt) == 1 && ImgFmt_NBits(Fmt) == 8);

	ccv_dense_matrix_t* mat = ccv_dense_matrix_new(Height, Width, CCV_8U | CCV_C1, nullptr, 0);
	for (int y = 0; y < Height; y++)
		memcpy(mat->data.u8 + mat->step * y, RowPtr(y), LineBytes());

	return mat;
}
#endif

#ifdef SX_OPENCV
cv::Mat Image::ToOpenCV_NoCopy() const
{
	assert(ImgFmt_NBits(Fmt) == 8);
	return cv::Mat(Height, Width, CV_MAKETYPE(CV_8U, ImgFmt_NChan(Fmt)), Scan0, Stride);
}
#endif


}