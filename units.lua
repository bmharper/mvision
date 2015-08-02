require "tundra.syntax.glob"
require "tundra.syntax.files"

local function ccv_path(config, file)
	return "third_party/t2-output/win64-mingw-" .. config .. "-default/" .. file
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

local WebcamShow = Program {
	Name = "WebcamShow",
	SourceDir = "src/",
	Depends = { xo_env, xo, crt },
	Env = {
		LIBPATH = {
			{ ccv_path("debug", "");   Config = "win64-*-debug-default" },
			{ ccv_path("release", ""); Config = "win64-*-release-default" },
		},
	},
	Libs = { "mfplat.lib", "mf.lib", "mfreadwrite.lib", "mfuuid.lib", "d3d9.lib", "shlwapi.lib", "user32.lib", "ole32.lib", "libccv.lib", "opengl32.lib", "user32.lib", "gdi32.lib", },
	Includes = { "src/", "../", ".", }, -- the ../ is for /xo/...
	PrecompiledHeader = {
		Source = "src/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		"xoMain.cpp",
		--"VideoCapture.cpp",
		"WebcamShow.cpp",
		"Tracker.cpp",
		"Tracker.h",
		"win_capture/BufferLock.h",
		"win_capture/capture_device.cpp",
		"win_capture/capture_device.h",
		"win_capture/device.cpp",
		"win_capture/device.h",
		"win_capture/MFCaptureD3D.h",
		"win_capture/preview.cpp",
		"win_capture/preview.h",
	},
}

-- Copy a file with source/target that can vary per configuration 
--local copy_1 = CopyFile { Source = 'a', Target = '$(OBJECTDIR)/a' }

-- Copy a file that is the same across all configurations
--local copy_2 = CopyFileInvariant { Source = 'a', Target = 'b' }

local copy_libccv = CopyFile {
	Source = ccv_path("$(CURRENT_VARIANT)", "libccv.dll"),
	Target = '$(OBJECTDIR)/libccv.dll'
}
Default(copy_libccv)

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
