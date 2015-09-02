#include "pch.h"
#include "Common.h"
#include "Tracker.h"

#ifdef SX_CCV

namespace sx
{

Tracker::Tracker()
{
}

Tracker::~Tracker()
{
	if (PrevMatrix)
		ccv_matrix_free(PrevMatrix);
}

void Tracker::Initialize(Image* data, ccv_rect_t box)
{
	Width = data->Width;
	Height = data->Height;
	box.x /= Scale;
	box.y /= Scale;
	box.width /= Scale;
	box.height /= Scale;
	PrevMatrix = RGB24_to_Lum8_Scaled(data);
	TLD = ccv_tld_new(PrevMatrix, box, *ccv_tld_default_params_get());
}

void Tracker::Track(Image* data, ccv_comp_t& newbox, ccv_tld_info_t& info)
{
	ccv_dense_matrix_t* newMatrix = RGB24_to_Lum8_Scaled(data);
	newbox = ccv_tld_track_object(TLD, PrevMatrix, newMatrix, &info);
	newbox.rect.x *= Scale;
	newbox.rect.y *= Scale;
	newbox.rect.width *= Scale;
	newbox.rect.height *= Scale;
	ccv_matrix_free(PrevMatrix);
	PrevMatrix = newMatrix;
}

ccv_dense_matrix_t* Tracker::RGB24_to_Lum8_Scaled(Image* data)
{
	Image* lum8 = data->Clone(ImgFmt::Lum8u);
	for (int scale = Scale; scale != 1; scale /= 2)
	{
		Image* next = lum8->HalfSize_Box(false);
		delete lum8;
		lum8 = next;
	}
	ccv_dense_matrix_t* res = lum8->ToCCV();
	delete lum8;
	return res;
}

}

#endif