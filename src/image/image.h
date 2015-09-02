#pragma once

namespace sx
{

enum class ImgFmt : uint32_t
{
	Null = 0,
	Lum8u = (1 << 8) | 8,
	Lum32f = (1 << 8) | 32,
	RGB8u = (3 << 8) | 8,
	RGBA8u = (4 << 8) | 8,
};

inline int ImgFmt_NChan(ImgFmt f)		{ return (((uint32_t) f) & 0xff00) >> 8; }
inline int ImgFmt_NBits(ImgFmt f)		{ return ((uint32_t) f) & 0xff; }
inline int ImgFmt_BytesPP(ImgFmt f)		{ return ImgFmt_NChan(f) * ImgFmt_NBits(f) / 8; }
inline int ImgStride(int rawLenBytes)	{ return (rawLenBytes + 3) & ~3; }

class Image
{
public:
	void*	Buf = nullptr;
	void*	Scan0 = nullptr;
	ImgFmt	Fmt = ImgFmt::Null;
	int		Width = 0;
	int		Height = 0;
	int		Stride = 0;

	~Image();
	void	Free();
	bool	Alloc(ImgFmt fmt, int width, int height);
	void	CopyTo(Image* dst) const;
	Image*	Clone(ImgFmt dstFormat = ImgFmt::Null) const;
	Image* 	HalfSize_Box(bool sRGB);
	Image* 	HalfSize_Box_Until(bool sRGB, int widthLessThanOrEqualTo);

#ifdef SX_CCV
	ccv_dense_matrix_t*	ToCCV() const;
#endif

#ifdef SX_OPENCV
	cv::Mat ToOpenCV_NoCopy() const;
#endif

	void*	RowPtr(int row) const { return (byte*) Scan0 + row * Stride; }
	uint8*	RowPtr8u(int row) const { return (uint8*) Scan0 + row * Stride; }
	float*	RowPtr32f(int row) const { return (float*) ((uint8*) Scan0 + row * Stride); }
	int		LineBytes() const { return ImgFmt_BytesPP(Fmt) * Width; }
};

}