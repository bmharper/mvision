require "tundra.syntax.glob"
require "tundra.syntax.files"
require "tundra.util"

-- Global options
local enableStaticCrt = false
local enableCCV_MinGW = true
local enableOpenCV = true

local function ccv_path(config, file)
	return "third_party/t2-output/win64-mingw-" .. config .. "-default/" .. file
end

local function opencv_path()
	return "C:/dev/tools/opencv/build/x64/vc12/lib/"
end

local function xo_path(config, file)
	return "../xo/t2-output/win64-msvc2013-" .. config .. "-default/" .. file
end

local winKernelLibs = { "kernel32.lib", "user32.lib", "gdi32.lib", "winspool.lib", "advapi32.lib", "shell32.lib", "comctl32.lib", 
						"uuid.lib", "ole32.lib", "oleaut32.lib", "shlwapi.lib", "OLDNAMES.lib", "wldap32.lib", "wsock32.lib",
						"Psapi.lib", "Msimg32.lib", "Comdlg32.lib", "RpcRT4.lib", "Iphlpapi.lib", "Delayimp.lib" }

local winDebugFilter = "win*-*-debug"
local winReleaseFilter = "win*-*-release"

-- Dynamic (msvcr110.dll etc) CRT linkage
local winLibsDynamicCRTDebug = tundra.util.merge_arrays( { "msvcrtd.lib", "msvcprtd.lib", "comsuppwd.lib" }, winKernelLibs )
local winLibsDynamicCRTRelease = tundra.util.merge_arrays( { "msvcrt.lib", "msvcprt.lib", "comsuppw.lib" }, winKernelLibs )

winLibsDynamicCRTDebug.Config = winDebugFilter
winLibsDynamicCRTRelease.Config = winReleaseFilter

-- Static CRT linkage
local winLibsStaticCRTDebug = tundra.util.merge_arrays( { "libcmtd.lib", "libcpmtd.lib", "comsuppwd.lib" }, winKernelLibs )
local winLibsStaticCRTRelease = tundra.util.merge_arrays( { "libcmt.lib", "libcpmt.lib", "comsuppw.lib" }, winKernelLibs )

winLibsStaticCRTDebug.Config = winDebugFilter
winLibsStaticCRTRelease.Config = winReleaseFilter

local winDynamicOpts = {
	{ "/MDd";					Config = winDebugFilter },
	{ "/MD";					Config = winReleaseFilter },
}

local winStaticOpts = {
	{ "/MTd";					Config = winDebugFilter },
	{ "/MT";					Config = winReleaseFilter },
}

local winDynamicEnv = {
	CCOPTS = winDynamicOpts,
	CXXOPTS = winDynamicOpts,
}

local winStaticEnv = {
	CCOPTS = winStaticOpts,
	CXXOPTS = winStaticOpts,
}

local crtDynamic = ExternalLibrary {
	Name = "crtdynamic",
	Propagate = {
		Env = winDynamicEnv,
		Libs = {
			winLibsDynamicCRTDebug,
			winLibsDynamicCRTRelease,
		},
	},
}

local crtStatic = ExternalLibrary {
	Name = "crtstatic",
	Propagate = {
		Env = winStaticEnv,
		Libs = {
			winLibsStaticCRTDebug,
			winLibsStaticCRTRelease,
		},
	},
}

local crt = crtDynamic
if enableStaticCrt then
	crt = crtStatic
end

-- I'm giving up on libccv until I can get it compiling natively on Windows.
-- I don't know when or why it happened, but at some point I just started 
-- getting exceptions inside any call to libccv, when I was compiling it
-- with mingw (TDM package of GCC 5.1.0).
--[[
local ccv = SharedLibrary {
	Name = "ccv",
	Libs = { "ws2_32", },
	SourceDir = "third_party/ccv/",
	Sources = {
		-- Glob { Dir = "ccv/lib", Extensions = { ".c", ".h" }, },
		--"lib/ccv.h-$(CXXOPTS_$(CURRENT_VARIANTX:u))",
		"lib/ccv.h",
		"lib/ccv_algebra.c",
		"lib/ccv_basic.c",
		"lib/ccv_bbf.c",
		"lib/ccv_cache.c",
		"lib/ccv_classic.c",
		"lib/ccv_daisy.c",
		"lib/ccv_dpm.c",
		"lib/ccv_ferns.c",
		"lib/ccv_internal.h",
		"lib/ccv_io.c",
		"lib/ccv_memory.c",
		"lib/ccv_mser.c",
		"lib/ccv_numeric.c",
		"lib/ccv_resample.c",
		"lib/ccv_sift.c",
		"lib/ccv_sparse_coding.c",
		"lib/ccv_swt.c",
		"lib/ccv_tld.c",
		"lib/ccv_transform.c",
		"lib/ccv_util.c",
		"lib/3rdparty/dsfmt/dSFMT.c",
		"lib/3rdparty/dsfmt/dSFMT.h",
		"lib/3rdparty/dsfmt/dSFMT-common.h",
		"lib/3rdparty/dsfmt/dSFMT-params.h",
		"lib/3rdparty/dsfmt/dSFMT-params19937.h",
		"lib/3rdparty/kissfft/_kiss_fft_guts.h",
		"lib/3rdparty/kissfft/_kissf_fft_guts.h",
		"lib/3rdparty/kissfft/kiss_fft.c",
		"lib/3rdparty/kissfft/kiss_fft.h",
		"lib/3rdparty/kissfft/kiss_fftnd.c",
		"lib/3rdparty/kissfft/kiss_fftnd.h",
		"lib/3rdparty/kissfft/kiss_fftndr.c",
		"lib/3rdparty/kissfft/kiss_fftndr.h",
		"lib/3rdparty/kissfft/kiss_fftr.c",
		"lib/3rdparty/kissfft/kiss_fftr.h",
		"lib/3rdparty/kissfft/kissf_fft.c",
		"lib/3rdparty/kissfft/kissf_fft.h",
		"lib/3rdparty/kissfft/kissf_fftnd.c",
		"lib/3rdparty/kissfft/kissf_fftnd.h",
		"lib/3rdparty/kissfft/kissf_fftndr.c",
		"lib/3rdparty/kissfft/kissf_fftndr.h",
		"lib/3rdparty/kissfft/kissf_fftr.c",
		"lib/3rdparty/kissfft/kissf_fftr.h",
		"lib/3rdparty/sfmt/SFMT.c",
		"lib/3rdparty/sfmt/SFMT.h",
		"lib/3rdparty/sfmt/SFMT-alti.h",
		"lib/3rdparty/sfmt/SFMT-common.h",
		"lib/3rdparty/sfmt/SFMT-params.h",
		"lib/3rdparty/sfmt/SFMT-params19937.h",
		"lib/3rdparty/sfmt/SFMT-sse2.h",
		"lib/3rdparty/sha1/sha1.c",
		"lib/3rdparty/sha1/sha1.h",
		--"lib/io/_ccv_io_binary.c",
		--"lib/io/_ccv_io_bmp.c",
		--"lib/io/_ccv_io_libjpeg.c",
		--"lib/io/_ccv_io_libpng.c",
		--"lib/io/_ccv_io_raw.c",
	},
}
--]]


local xo_use_amalgamation = false
local xo, xo_env

if xo_use_amalgamation then
	xo_env = ExternalLibrary {
		Name = "xo_env",
		Propagate = {
			Defines = {
				"MVISION_USE_XO_AMALGAMATION=1"
			},
		},
	}
	xo = SharedLibrary {
		Name = "xo",
		Libs = { "opengl32.lib", "user32.lib", "gdi32.lib", "shell32.lib", "D3D11.lib", "d3dcompiler.lib", },
		Sources = {
			"../xo/amalgamation/xo-amalgamation.cpp",
			"../xo/amalgamation/xo-amalgamation-freetype.c",
		},
	}
else
	xo_env = ExternalLibrary {
		Name = "xo_env",
		Propagate = {
			Defines = {
				"MVISION_USE_XO_AMALGAMATION=0"
			},
		},
	}
	xo = ExternalLibrary {
		Name = "xo",
		Propagate = {
			Libs = {
				"xo.lib"
			},
			Env = {
				LIBPATH = {
					{ xo_path("debug", "");   Config = "win64-*-debug-default" },
					{ xo_path("release", ""); Config = "win64-*-release-default" },
				},
			},
		},
	}

	local copy_xo_lib = CopyFile {
		Source = xo_path("$(CURRENT_VARIANT)", "xo.dll"),
		Target = '$(OBJECTDIR)/xo.dll'
	}
	Default(copy_xo_lib)
end

local HelloWorld = Program {
	Name = "HelloWorld",
	Sources = {
		"src/HelloWorld.cpp",
	},
}

local libccv_libs = {}
local libccv_defines = {}
if enableCCV_MinGW then
	libccv_libs = { "libccv.lib", "Delayimp.lib" }
	libccv_defines = { "SX_CCV" }
end

local opencv_libs = {}
local opencv_defines = {}
if enableOpenCV then
	opencv_libs = {
		{ "opencv_world300d.lib"; Config = "win*-*-debug-*" },
		{ "opencv_world300.lib"; Config = "win*-*-release-*" },
	}
	opencv_defines = { "SX_OPENCV" }
end

local WebcamShow = Program {
	Name = "WebcamShow",
	SourceDir = "src/",
	Depends = { xo_env, xo, crt },
	Defines = tundra.util.merge_arrays(libccv_defines, opencv_defines),
	Env = {
		LIBPATH = {
			{ ccv_path("debug", "");   Config = "win64-*-debug-default" },
			{ ccv_path("release", ""); Config = "win64-*-release-default" },
			{ opencv_path();           Config = "win*" },
		},
		PROGOPTS = {
			"/DELAYLOAD:libccv.dll"; Config = "win*", 
		},
	},
	Libs = tundra.util.merge_arrays(
		{ "mfplat.lib", "mf.lib", "mfreadwrite.lib", "mfuuid.lib", "d3d9.lib", "shlwapi.lib", "user32.lib", "ole32.lib", "opengl32.lib", "user32.lib", "gdi32.lib" },
		libccv_libs,
		opencv_libs ),
	Includes = {
		"src/",
		"../",		-- for /xo/...
		".",
		"c:/dev/tools/opencv/build/include"
	}, 
	PrecompiledHeader = {
		Source = "src/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		"Common.cpp",
		"Common.h",
		"MotionDetect.cpp",
		"MotionDetect.h",
		"xoMain.cpp",
		--"VideoCapture.cpp",
		"WebcamShow.cpp",
		"Tracker.cpp",
		"Tracker.h",
		"TestBackgroundSubtractor.cpp",
		"TestMotion.cpp",
		"TestORB.cpp",
		"TestSIFT.cpp",
		"TestTLD.cpp",
		"cameras/Cameras.h",
		"cameras/MJPEG.cpp",
		"cameras/MJPEG.h",
		"cameras/win_capture/BufferLock.h",
		"cameras/win_capture/capture_device.cpp",
		"cameras/win_capture/capture_device.h",
		"cameras/win_capture/device.cpp",
		"cameras/win_capture/device.h",
		"cameras/win_capture/MFCaptureD3D.h",
		"cameras/win_capture/preview.cpp",
		"cameras/win_capture/preview.h",
		"image/image.cpp",
		"image/image.h",
	},
}

-- Copy a file with source/target that can vary per configuration 
--local copy_1 = CopyFile { Source = 'a', Target = '$(OBJECTDIR)/a' }

-- Copy a file that is the same across all configurations
--local copy_2 = CopyFileInvariant { Source = 'a', Target = 'b' }

-- This was used when building libccv via an external project that uses mingw toolchain
if enableCCV_MinGW then
	local copy_libccv = CopyFile {
		Source = ccv_path("$(CURRENT_VARIANT)", "libccv.dll"),
		Target = '$(OBJECTDIR)/libccv.dll'
	}
	Default(copy_libccv)
end

if enableOpenCV then
	local copy_opencv_debug = CopyFile {
		Source = "C:/dev/tools/opencv/build/x64/vc12/bin/opencv_world300d.dll",
		Target = "$(OBJECTDIR)/opencv_world300d.dll"
	}
	Default(copy_opencv_debug)

	local copy_opencv_release = CopyFile {
		Source = "C:/dev/tools/opencv/build/x64/vc12/bin/opencv_world300.dll",
		Target = "$(OBJECTDIR)/opencv_world300.dll"
	}
	Default(copy_opencv_release)
end


--local playcap = Program {
--	Name = "playcap",
--	Libs = { "quartz.lib" },
--	Sources = {
--		"src/playcap.cpp",
--	},
--}

--Default(xo)
--Default(playcap)
Default(WebcamShow)
