#include "pch.h"
#include "Common.h"
#include "Tracker.h"

Tracker::Tracker()
{
}

Tracker::~Tracker()
{
	if (PrevMatrix)
		ccv_matrix_free(PrevMatrix);
}

void Tracker::Initialize(int width, int height, void* data, ccv_rect_t box)
{
	Width = width;
	Height = height;
	box.x /= Scale;
	box.y /= Scale;
	box.width /= Scale;
	box.height /= Scale;
	PrevMatrix = RGB24_to_Lum8_Scaled(width, height, data);
	TLD = ccv_tld_new(PrevMatrix, box, *ccv_tld_default_params_get());
}

void Tracker::Track(void* data, ccv_comp_t& newbox, ccv_tld_info_t& info)
{
	ccv_dense_matrix_t* newMatrix = RGB24_to_Lum8_Scaled(Width, Height, data);
	newbox = ccv_tld_track_object(TLD, PrevMatrix, newMatrix, &info);
	newbox.rect.x *= Scale;
	newbox.rect.y *= Scale;
	newbox.rect.width *= Scale;
	newbox.rect.height *= Scale;
	ccv_matrix_free(PrevMatrix);
	PrevMatrix = newMatrix;
}

ccv_dense_matrix_t* Tracker::RGB24_to_Lum8_Scaled(int width, int height, void* data)
{
	ccv_dense_matrix_t* mat = Util_RGB_to_CCV_Lum8(width, height, data);
	for (int scale = Scale; scale != 1; scale /= 2)
	{
		ccv_dense_matrix_t* next = Util_Lum_HalfSize_Box(mat, false);
		ccv_matrix_free(mat);
		mat = next;
	}
	return mat;
}
