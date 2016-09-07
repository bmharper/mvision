
local common = {
	Env = {
		CPPPATH = {
			-- "third_party/spdlog/include",
		}
	}
}

local win_linker = {
	{ "/NXCOMPAT /DYNAMICBASE";		Config = "win*" },
}

local win_common = {
	Inherit = common,
	Env = {
		CXXOPTS = {
			{ "/EHsc"; Config = "win*" },
		},
		PROGOPTS = win_linker,
		SHLIBOPTS = win_linker,
		GENERATE_PDB = {
			{ "1"; Config = "win*" },
		},
		INCREMENTAL = {
			{ "1";  Config = "win*-*-debug" },
		},
	},
}

Build {
	Units = "units.lua",
	Passes= {
		PchGen = { Name = "Precompiled Header Generation", BuildOrder = 1 },
	},
	Configs = {
		{
			Name = "macosx-gcc",
			DefaultOnHost = "macosx",
			Tools = { "gcc" },
			Inherit = common,
		},
		{
			Name = "linux-gcc",
			DefaultOnHost = "linux",
			Tools = { "gcc" },
			Inherit = common,
		},
		{
			Name = "win32-msvc",
			--DefaultOnHost = "windows",
			SupportedHosts = { "windows" },
			Inherit = win_common,
			Tools = { {"msvc-vs2013"; TargetArch = "x86"} },
		},
		{
			Name = "win64-msvc",
			DefaultOnHost = "windows",
			Inherit = win_common,
			Tools = { {"msvc-vs2013"; TargetArch = "x64"} },
		},
		{
			Name = "win64-mingw",
			SupportedHosts = { "windows" },
			Tools = { "mingw" },
			Inherit = common,
			-- Link with the C++ compiler to get the C++ standard library.
			ReplaceEnv = {
    			PCHCOMPILE = "$(CC) $(_OS_CCOPTS) -c $(CPPDEFS:p-D) $(CPPPATH:f:p-I) $(CCOPTS) $(CCOPTS_$(CURRENT_VARIANT:u)) -o $(@) $(<)",
    			PCHCOMPILE_CXX = "$(CXX) $(_OS_CXXOPTS) -c $(CPPDEFS:p-D) $(CPPPATH:f:p-I) $(CXXOPTS) $(CXXOPTS_$(CURRENT_VARIANT:u)) -o $(@) $(<)",
				LD = "$(CXX)",
			},
		},
	},
	IdeGenerationHints = {
		Msvc = {
			-- Remap config names to MSVC platform names (affects things like header scanning & debugging)
			PlatformMappings = {
				['win64-msvc'] = 'x64',
				['win32-msvc'] = 'Win32',
				['win64-mingw'] = 'MinGW-64',
			},
			-- Remap variant names to MSVC friendly names
			VariantMappings = {
				['release-default']    = 'Release',
				['debug-default']      = 'Debug',
				['release-analyze']    = 'Release Analyze',
				['debug-analyze']      = 'Debug Analyze',
			},
		},
		-- Override solutions to generate and what units to put where.
		MsvcSolutions = {
			['mvision.sln'] = {
				"HelloWorld",
				"live555",
				"yuv",
				"WebcamShow",
			},
		},
		BuildAllByDefault = true,
	},
}
