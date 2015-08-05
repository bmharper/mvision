#pragma once

class Tracker
{
public:

	Tracker();
	~Tracker();
	void					Initialize(int width, int height, void* data, ccv_rect_t box);
	void					Track(void* data, ccv_comp_t& newbox, ccv_tld_info_t& info);

private:
	int64					MatrixSig;
	int						Width;
	int						Height;
	ccv_dense_matrix_t		Matrix;
	ccv_tld_t*				TLD;

	void*					Half_RGB24_to_Lum8(int width, int height, void* data);
	void*					RGB24_to_Lum8(int width, int height, void* data);
	void*					Half_Lum8(int width, int height, void* data);
};