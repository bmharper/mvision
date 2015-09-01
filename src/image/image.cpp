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

}