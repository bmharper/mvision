#pragma once

#define NOMINMAX
#define SX_LIVE555

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include <shlwapi.h>

#if MVISION_USE_XO_AMALGAMATION
#	define XO_AMALGAMATION
#	include <xo/amalgamation/xo-amalgamation.h>
#else
#	include <xo/xo/xo.h>
#endif

//#include <VideoCapture.h>

#ifdef SX_INTEL_MEDIA_SDK
#include <mfxvideo.h>
#endif

#include <stdint.h>

#include <windowsx.h>
#include <d3d9.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>

#include <strsafe.h>
#include <assert.h>

#include <ks.h>
#include <ksmedia.h>
#include <Dbt.h>

#include <chrono>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}


//#include "win_capture/MFCaptureD3D.h"

#define __attribute__(x)
extern "C" {
#include "third_party/ccv/lib/ccv.h"
}
#undef __attribute__

#ifdef SX_OPENCV
#include <opencv2/features2d.hpp>
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>
#endif

#ifdef SX_LIVE555
#include "third_party/live555/liveMedia/include/liveMedia.hh"
#include "third_party/live555/BasicUsageEnvironment/include/BasicUsageEnvironment.hh"
#endif

#include "third_party/microlog.h"
#include "third_party/ring.h"

#include "third_party/yuv/include/libyuv.h"

#include "Sys.h"
#include "util/Error.h"
