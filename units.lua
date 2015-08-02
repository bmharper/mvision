require "tundra.syntax.glob"
require "tundra.syntax.files"

local function ccv_path(config, file)
	return "third_party/t2-output/win64-mingw-" .. config .. "-default/" .. file
end

local nudom = SharedLibrary {
	Name = "nudom",
	Libs = { "opengl32.lib", "user32.lib", "gdi32.lib" },
	SourceDir = "../nudom/nudom/",
	Includes = { "../nudom/nudom/" },
	PrecompiledHeader = {
		Source = "../nudom/nudom/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		--Glob { Dir = "../nudom/nudom/", Extensions = { ".h" }, Recursive = true },
		"nuDefs.cpp",
		"nuDoc.cpp",
		"nuDomEl.cpp",
		"nuEvent.cpp",
		"nuLayout.cpp",
		"nuMem.cpp",
		"nuQueue.cpp",
		"nuPlatform.cpp",
		"nuString.cpp",
		"nuStringTable.cpp",
		"nuStyle.cpp",
		"nuStyleParser.cpp",
		"nuSysWnd.cpp",
		"nuProcessor.cpp",
		"nuProcessor_Win32.cpp",
		"Image/nuImage.cpp",
		"Image/nuImageStore.cpp",
		"Render/nuRenderer.cpp",
		"Render/nuRenderGL.cpp",
		"Render/nuRenderDoc.cpp",
		"Render/nuRenderDomEl.cpp",
		"Render/nuStyleResolve.cpp",
		"Text/nuTextCache.cpp",
		"Text/nuTextDefs.cpp",
		"../dependencies/Panacea/Containers/queue.cpp",
		"../dependencies/Panacea/Platform/syncprims.cpp",
		"../dependencies/Panacea/Platform/err.cpp",
		"../dependencies/Panacea/Strings/fmt.cpp",
		"../dependencies/glext.cpp",
	},
}

local HelloWorld = Program {
	Name = "HelloWorld",
	Sources = {
		"src/HelloWorld.cpp",
	},
}

local WebcamShow = Program {
	Name = "WebcamShow",
	SourceDir = "src/",
	Depends = { nudom },
	Env = {
		LIBPATH = {
			{ ccv_path("debug", "");   Config = "win64-*-debug-default" },
			{ ccv_path("release", ""); Config = "win64-*-release-default" },
		},
	},
	Libs = { "mfplat.lib", "mf.lib", "mfreadwrite.lib", "mfuuid.lib", "d3d9.lib", "shlwapi.lib", "user32.lib", "ole32.lib", "libccv.lib", },
	Includes = { "src/", "../nudom", ".", },
	PrecompiledHeader = {
		Source = "src/pch.cpp",
		Header = "pch.h",
		Pass = "PchGen",
	},
	Sources = {
		"nuMain.cpp",
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

Default(nudom)
--Default(playcap)
Default(WebcamShow)
