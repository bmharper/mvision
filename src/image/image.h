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
	enum { MemAlignment = 16 }; // memory alignment is mandated by Intel's Media SDK

	void*	Buf = nullptr;
	void*	Scan0 = nullptr;
	ImgFmt	Fmt = ImgFmt::Null;
	int		Width = 0;
	int		Height = 0;
	int		Stride = 0;

	~Image();
	void	Free();
	bool	Alloc(ImgFmt fmt, int width, int height);
	void	FillBytes(uint8 byteVal) const;
	void	CopyTo(Image* dst) const;
	Image*	Clone(ImgFmt dstFormat = ImgFmt::Null) const;
	void	FixBGRA_to_RGBA();
	Image* 	HalfSize_Box(bool sRGB);
	Image* 	HalfSize_Box_Until(bool sRGB, int widthLessThanOrEqualTo);

#ifdef SX_CCV
	ccv_dense_matrix_t*	ToCCV() const;
#endif

#ifdef SX_OPENCV
	cv::Mat ToOpenCV_NoCopy() const;
	static Image* FromOpenCV(cv::Mat mat);
#endif

	void*	RowPtr(int row) const { return (byte*) Scan0 + row * Stride; }
	uint8*	RowPtr8u(int row) const { return (uint8*) Scan0 + row * Stride; }
	uint32*	RowPtr32u(int row) const { return (uint32*) ((uint8*) Scan0 + row * Stride); }
	float*	RowPtr32f(int row) const { return (float*) ((uint8*) Scan0 + row * Stride); }
	int		LineBytes() const { return ImgFmt_BytesPP(Fmt) * Width; }
};

}