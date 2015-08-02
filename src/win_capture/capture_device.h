#pragma once


class CaptureDevice : public IMFSourceReaderCallback
{
public:
							CaptureDevice();
							~CaptureDevice();
	bool					InitializeFirst( std::string& error );
	void					Close();
	void*					GetNextFrame(); // The caller must free() this buffer finished with it.
	uint32					GetWidth() { return InputWidth; }
	uint32					GetHeight() {  return InputHeight; }

    // IUnknown methods
    STDMETHODIMP			QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG)	AddRef();
    STDMETHODIMP_(ULONG)	Release();

    // IMFSourceReaderCallback methods
    STDMETHODIMP			OnReadSample( HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample );
    STDMETHODIMP			OnEvent( DWORD, IMFMediaEvent * );
    STDMETHODIMP			OnFlush( DWORD);

protected:
	LPWSTR					SymbolicLink;
	ULONG					RefCount;
    IMFSourceReader*		Reader;
	GUID					InputType;
	UINT32					InputWidth;
	UINT32					InputHeight;
	LONG					InputDefaultStride;
	MFVideoInterlaceMode	InputInterlaceMode;
	TAbcQueue<void*>		Frames;
	int						MaxFrames;
	uint32					EnableCapture;

	static IMFActivate*		ChooseFirst( std::string& error );
	static std::string		ErrorMessage( PCWSTR format, HRESULT hrErr );
	static bool				IsMediaTypeSupported( IMFMediaType* pType, GUID& typeId );
	static HRESULT			GetDefaultStride( IMFMediaType* pType, LONG* plStride );

};