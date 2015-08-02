#include "pch.h"
#include "Tracker.h"

// Use 'div.exe' by AMD to calculate these things. Valid ranges are up to 32k or 64k, I'm not sure.
inline uint32 DivideByThree( uint32 i )
{
	i *= 0xAAAB;
	return i >> 17;
}

Tracker::Tracker()
{
	MatrixSig = 1;
	memset( &Matrix, 0, sizeof(Matrix) );
}

Tracker::~Tracker()
{
	free( Matrix.data.u8 );
}

void Tracker::Initialize( int width, int height, void* data, ccv_rect_t box )
{
	Width = width;
	Height = height;
	box.x /= 2;
	box.y /= 2;
	box.width /= 2;
	box.height /= 2;
	Matrix = ccv_dense_matrix( Height / 2, Width / 2, CCV_8U | CCV_C1, Half_RGB24_to_Lum8( width, height, data ), MatrixSig++ );
	TLD = ccv_tld_new( &Matrix, box, *ccv_tld_default_params_get() );
}

void Tracker::Track( void* data, ccv_comp_t& newbox, ccv_tld_info_t& info )
{
	ccv_dense_matrix_t newMatrix = ccv_dense_matrix( Height / 2, Width / 2, CCV_8U | CCV_C1, Half_RGB24_to_Lum8( Width, Height, data ), MatrixSig++ );
	newbox = ccv_tld_track_object( TLD, &Matrix, &newMatrix, &info );
	newbox.rect.x *= 2;
	newbox.rect.y *= 2;
	newbox.rect.width *= 2;
	newbox.rect.height *= 2;
	free( Matrix.data.u8 );
	Matrix = newMatrix;
}

void* Tracker::Half_RGB24_to_Lum8( int width, int height, void* data )
{
	void* lum = RGB24_to_Lum8( width, height, data );
	void* half = Half_Lum8( width, height, lum );
	free( lum );
	return half;
}

void* Tracker::RGB24_to_Lum8( int width, int height, void* data )
{
	void* dst = malloc( width * height );
	for ( int y = 0; y < height; y++ )
	{
		uint8* srcLine = ((uint8*) data) + y * width * 3;
		uint8* dstLine = ((uint8*) dst) + y * width;
		for ( int x = 0, x3 = 0; x < width; x++, x3 += 3 )
		{
			dstLine[x] = DivideByThree( srcLine[x3] + srcLine[x3 + 1] + srcLine[x3 + 2] );
		}
	}
	return dst;
}

void* Tracker::Half_Lum8( int width, int height, void* data )
{
	int nwidth = width / 2;
	int nheight = height / 2;
	void* dst = malloc( nwidth * nheight );
	for ( int y = 0; y < nheight; y++ )
	{
		uint8* srcLine1 = ((uint8*) data) + (y << 1) * width;
		uint8* srcLine2 = ((uint8*) data) + ((y << 1) + 1) * width;
		uint8* dstLine = ((uint8*) dst) + y * nwidth;
		for ( int x = 0; x < nwidth; x++ )
		{
			int x2 = x << 1;
			uint32 sum1 = srcLine1[x2] + srcLine1[x2 + 1];
			uint32 sum2 = srcLine2[x2] + srcLine2[x2 + 1];
			dstLine[x] = sum1 + sum2;
		}
	}
	return dst;
}
