#pragma once

#ifdef SX_CCV

namespace sx
{

class Tracker
{
public:
	// Scale down image before processing by 2x, 4x, 8x, (must be a power of 2)
	int						Scale = 2;

	Tracker();
	~Tracker();
	void					Initialize(Image* data, ccv_rect_t box);
	void					Track(Image* data, ccv_comp_t& newbox, ccv_tld_info_t& info);

private:
	int						Width = 0;
	int						Height = 0;
	ccv_dense_matrix_t*		PrevMatrix = nullptr;
	ccv_tld_t*				TLD = nullptr;

	ccv_dense_matrix_t*		RGB24_to_Lum8_Scaled(Image* data);
};

}

#endif