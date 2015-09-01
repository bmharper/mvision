#include "pch.h"
#include "capture_device.h"
#include "BufferLock.h"
#include "../../image/image.h"

namespace sx
{

CaptureDevice::CaptureDevice()
{
	SymbolicLink = NULL;
	RefCount = 1;
	EnableCapture = 1;
	Reader = NULL;
	MaxFrames = 3;
	Frames.Initialize(false);
}

CaptureDevice::~CaptureDevice()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ICamera methods

int CaptureDevice::Width()
{
	return InputWidth;
}

int CaptureDevice::Height()
{
	return InputHeight;
}

Image* CaptureDevice::NextFrame()
{
	Image* next = nullptr;
	Frames.PopTail(next);
	return next;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IUnknown methods

ULONG CaptureDevice::AddRef()
{
	return InterlockedIncrement(&RefCount);
}

ULONG CaptureDevice::Release()
{
	ULONG uCount = InterlockedDecrement(&RefCount);
	if (uCount == 0)
		delete this;
	// For thread safety, return a temporary variable.
	return uCount;
}

HRESULT CaptureDevice::QueryInterface(REFIID riid, void** ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(CaptureDevice, IMFSourceReaderCallback),
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMFSourceReaderCallback methods

HRESULT CaptureDevice::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample)
{
	HRESULT hr = S_OK;
	IMFMediaBuffer *pBuffer = NULL;
	if (FAILED(hrStatus))
		hr = hrStatus;

	if (SUCCEEDED(hr))
	{
		if (pSample)
		{
			hr = pSample->GetBufferByIndex(0, &pBuffer);
			if (SUCCEEDED(hr))
			{
				VideoBufferLock vbuffer(pBuffer);
				BYTE* scan0 = NULL;
				LONG stride = 0;
				hr = vbuffer.LockBuffer(InputDefaultStride, InputHeight, &scan0, &stride);
				if (SUCCEEDED(hr))
				{
					Image* img = new Image();
					if (img->Alloc(ImgFmt::RGB8u, InputWidth, InputHeight))
					{
						byte* lineIn = scan0;
						byte* lineOut = (byte*) img->Scan0;
						for (int iline = 0; iline < (int) InputHeight; iline++)
						{
							memcpy(lineOut, lineIn, InputWidth * 3);
							lineIn += stride;
							lineOut += img->Stride;
						}
						Frames.Add(img);
						// This method of keeping the frame count down is subject to race conditions, but you've already screwed up here by losing frames,
						// so losing even more is not the end of the world.
						// 'delete' is safe to call on null, so we're OK even if the above mentioned race condition catches us.
						while (Frames.Size() > MaxFrames)
							delete NextFrame();
					}
					else
					{
						delete img;
					}
				}

				SafeRelease(&pBuffer);
			}
		}
	}

	// Request the next frame.
	if (SUCCEEDED(hr) && EnableCapture == 1)
		hr = Reader->ReadSample((DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);

	return hr;
}

HRESULT CaptureDevice::OnEvent(DWORD, IMFMediaEvent *)
{
	return S_OK;
}

HRESULT CaptureDevice::OnFlush(DWORD)
{
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CaptureDevice::Close()
{
	EnableCapture = 0;
	while (Frames.Size() != 0)
		free(NextFrame());
	SafeRelease(&Reader);
	CoTaskMemFree(SymbolicLink);
	SymbolicLink = NULL;
}

bool CaptureDevice::InitializeFirst(std::string& error)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (!SUCCEEDED(hr))
	{
		return false;
		error = "CoInitializeEx failed";
	}

	hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
	if (!SUCCEEDED(hr))
	{
		error = "MFStartup failed";
		return false;
	}

	Close();

	memset(&InputType, 0, sizeof(InputType));

	IMFActivate* activate = CaptureDevice::ChooseFirst(error);
	if (!activate)
		return false;

	IMFMediaSource  *pSource = NULL;
	IMFAttributes   *pAttributes = NULL;
	IMFMediaType    *pType = NULL;

	UINT32 m_cchSymbolicLink = 0;

	// Create the media source for the device.
	if (SUCCEEDED(hr))
		hr = activate->ActivateObject(__uuidof(IMFMediaSource), (void**) &pSource);

	// Get the symbolic link.
	if (SUCCEEDED(hr))
		hr = activate->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &SymbolicLink, &m_cchSymbolicLink);

	//
	// Create the source reader.
	//

	// Create an attribute store to hold initialization settings.

	if (SUCCEEDED(hr))
		hr = MFCreateAttributes(&pAttributes, 2);

	if (SUCCEEDED(hr))
		hr = pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);

	// Set the callback pointer.
	if (SUCCEEDED(hr))
		hr = pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);

	if (SUCCEEDED(hr))
		hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &Reader);

	// Try to find a suitable input type.
	if (SUCCEEDED(hr))
	{
		for (uint i = 0; ; i++)
		{
			hr = Reader->GetNativeMediaType((DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM, i, &pType);
			if (FAILED(hr))
			{
				error = "Failed to find a supported output format (ie RGB24)";
				break;
			}
			memset(&InputType, 0, sizeof(InputType));
			bool isTypeOK = IsMediaTypeSupported(pType, InputType);
			if (isTypeOK)
			{
				// Get the frame size.
				hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &InputWidth, &InputHeight);
				// Get the image stride.
				hr = GetDefaultStride(pType, &InputDefaultStride);
				// Get the interlace mode. Default: assume progressive.
				InputInterlaceMode = (MFVideoInterlaceMode) MFGetAttributeUINT32(pType, MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
			}
			SafeRelease(&pType);
			if (isTypeOK)
				break;
		}
	}

	if (SUCCEEDED(hr))
	{
		// Ask for the first sample.
		EnableCapture = 1;
		hr = Reader->ReadSample((DWORD) MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
	}

	if (FAILED(hr))
	{
		if (pSource)
		{
			pSource->Shutdown();
			// NOTE: The source reader shuts down the media source by default, but we might not have gotten that far.
		}
		Close();
	}

	SafeRelease(&pSource);
	SafeRelease(&pAttributes);
	SafeRelease(&pType);
	SafeRelease(&activate);

	if (FAILED(hr) && error.length() == 0)
		error = ErrorMessage(L"Failed to initialize video capture device", hr);

	return SUCCEEDED(hr);
}

bool CaptureDevice::IsMediaTypeSupported(IMFMediaType* pType, GUID& typeId)
{
	GUID subtype = { 0 };
	HRESULT hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
	if (FAILED(hr))
		return false;

	if (memcmp(&subtype, &MFVideoFormat_RGB24, sizeof(subtype)) == 0)
		return true;

	return false;
}

HRESULT CaptureDevice::GetDefaultStride(IMFMediaType* pType, LONG* plStride)
{
	LONG lStride = 0;

	// Try to get the default stride from the media type.
	HRESULT hr = pType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*) &lStride);
	if (FAILED(hr))
	{
		// Attribute not set. Try to calculate the default stride.
		GUID subtype = GUID_NULL;

		UINT32 width = 0;
		UINT32 height = 0;

		// Get the subtype and the image size.
		hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
		if (SUCCEEDED(hr))
		{
			hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
		}
		if (SUCCEEDED(hr))
		{
			hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &lStride);
		}

		// Set the attribute for later reference.
		if (SUCCEEDED(hr))
		{
			(void) pType->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(lStride));
		}
	}

	if (SUCCEEDED(hr))
	{
		*plStride = lStride;
	}
	return hr;
}

IMFActivate* CaptureDevice::ChooseFirst(std::string& error)
{
	IMFActivate* result = NULL;
	HRESULT hr = S_OK;
	UINT iDevice = 0;   // Index into the array of devices
	BOOL bCancel = FALSE;

	// Initialize an attribute store to specify enumeration parameters.
	IMFAttributes* pAttributes = NULL;
	hr = MFCreateAttributes(&pAttributes, 1);

	if (FAILED(hr)) { goto done; }

	// Ask for source type = video capture devices.

	hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

	if (FAILED(hr)) { goto done; }

	// Enumerate devices.
	IMFActivate **devices = NULL;
	uint numDevices = 0;
	hr = MFEnumDeviceSources(pAttributes, &devices, &numDevices);

	if (FAILED(hr)) { goto done; }

	if (numDevices > 0)
		result = devices[0];

done:

	SafeRelease(&pAttributes);

	for (uint i = 0; i < numDevices; i++)
	{
		if (devices[i] != result)
			SafeRelease(&devices[i]);
	}
	CoTaskMemFree(devices);

	if (FAILED(hr))
	{
		//ShowErrorMessage(L"Cannot create a video capture device", hr);
	}

	return result;
}

std::string CaptureDevice::ErrorMessage(PCWSTR format, HRESULT hrErr)
{
	HRESULT hr = S_OK;
	TCHAR msg[1024];
	msg[0] = 0;
	hr = StringCbPrintf(msg, sizeof(msg), _T("%s (hr=0x%X)"), format, hrErr);
	return msg;
}

}