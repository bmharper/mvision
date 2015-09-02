
local mingw_common = {
	Env = {
		CCOPTS_RELEASE = {
			-- On my SIFT example, O3 to O2 takes times from around 600ms closer to 500ms (but probably not
			-- as much as 100ms). -03 is MUCH slower though to compile.
			{ "-O2", "-ffast-math" },
		},
	},
}

Build {
	Units = "units.lua",
	Configs = {
		{
			Name = "macosx-gcc",
			DefaultOnHost = "macosx",
			Tools = { "gcc" },
		},
		{
			Name = "linux-gcc",
			DefaultOnHost = "linux",
			Tools = { "gcc" },
		},
		{
			Name = "win64-mingw",
			DefaultOnHost = "windows",
			Inherit = mingw_common,
			Tools = { "mingw" },
			-- Link with the C++ compiler to get the C++ standard library.
			ReplaceEnv = {
				LD = "$(CXX)",
			},
		},
	},
	IdeGenerationHints = {
	--[[
		Msvc = {
			-- Remap config names to MSVC platform names (affects things like header scanning & debugging)
			PlatformMappings = {
				['win64-msvc2010'] = 'x64',
				['win64-msvc2012'] = 'x64',
				['win64-msvc2013'] = 'x64',
				['win32-msvc2010'] = 'Win32',
				['win32-msvc2012'] = 'Win32',
				['win32-msvc2013'] = 'Win32',
			},
			-- Remap variant names to MSVC friendly names
			VariantMappings = {
				['release-default']    = 'Release',
				['debug-default']      = 'Debug',
				['release-analyze']    = 'Release Analyze',
				['debug-analyze']      = 'Debug Analyze',
			},
		},
		--]]
		-- Override solutions to generate and what units to put where.
		MsvcSolutions = {
			['ccv.sln'] = {}, -- receives all the units due to empty set
		},
		BuildAllByDefault = true,
	},
}
