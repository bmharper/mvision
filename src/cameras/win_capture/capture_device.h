#pragma once

#include "../Cameras.h"

namespace sx
{

class WinCaptureDevice : public IMFSourceReaderCallback, public ICamera
{
public:
	WinCaptureDevice();
	~WinCaptureDevice();
	bool					InitializeFirst(std::string& error);
	void					Close();

	// ICamera methods
	int						Width() override;
	int						Height() override;
	Image*					NextFrame() override;

	// IUnknown methods
	STDMETHODIMP			QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// IMFSourceReaderCallback methods
	STDMETHODIMP			OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample);
	STDMETHODIMP			OnEvent(DWORD, IMFMediaEvent *);
	STDMETHODIMP			OnFlush(DWORD);

protected:
	LPWSTR					SymbolicLink;
	ULONG					RefCount;
	IMFSourceReader*		Reader;
	GUID					InputType;
	UINT32					InputWidth;
	UINT32					InputHeight;
	LONG					InputDefaultStride;
	MFVideoInterlaceMode	InputInterlaceMode;
	TAbcQueue<Image*>		Frames;
	int						MaxFrames;
	uint32					EnableCapture;

	static IMFActivate*		ChooseFirst(std::string& error);
	static std::string		ErrorMessage(PCWSTR format, HRESULT hrErr);
	static bool				IsMediaTypeSupported(IMFMediaType* pType, GUID& typeId);
	static HRESULT			GetDefaultStride(IMFMediaType* pType, LONG* plStride);

};

}