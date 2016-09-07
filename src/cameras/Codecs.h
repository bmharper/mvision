#pragma once

#include "../util/Buffer.h"

namespace sx
{

class Image;

class H264Decoder
{
public:
	//ring::ByteBuf	Buf;

	virtual ~H264Decoder();
	virtual bool	Init() = 0;
	virtual size_t	AddData(const void* buf, size_t bufSize) = 0; // Return the number of bytes consumed from the stream
	//virtual void	Decode() = 0;
	virtual Image*	ReadFrame() = 0;
	virtual int		FramesInQueue() = 0;
};

#ifdef SX_INTEL_MEDIA_SDK

/* Use Intel's Media SDK to decode H264

We assume here that every time Decode() is called, we are given a full frame.
This is true when running behind live555. It simplifies the design. If that is not
the case, then we should expose a ring buffer here, and feed the mfxBitstream off
of our ring buffer.
*/
class IntelH264Decoder : public H264Decoder
{
public:
	IntelH264Decoder();
	~IntelH264Decoder() override;

	bool	Init() override;
	size_t	AddData(const void* buf, size_t bufSize) override;
	//void	Decode() override;
	Image*	ReadFrame() override;
	int		FramesInQueue() override;

private:
	ByteBuf				Buf;
	std::mutex			BufLock;	// Guards access to Buf, which is written by Live555 thread.
	std::vector<Image*> Frames;
	mfxSession			Session = nullptr;
	bool				IsInitialized = false;
	mfxVideoParam		VideoParam;
	mfxFrameSurface1	WorkSurface;

	size_t	Reset(const void* buf, size_t bufSize);
	//void	Reset();
	
	size_t	Decode(const void* buf, size_t bufSize);
	void	ConsumeBuf();
	void	CreateWorkSurface(mfxFrameSurface1& surf);

	//static void MakeBitstream(const ring::ByteBuf& rb, mfxBitstream& bs);
	//static void AdjustFromBitstream(const mfxBitstream& bs, ring::ByteBuf& rb);

};

#endif

}
