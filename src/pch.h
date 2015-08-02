#pragma once

#include <windows.h>
#include <stdio.h>

#include <shlwapi.h>

#include "nudom/nudom.h"
//#include <VideoCapture.h>

#include <windows.h>
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

