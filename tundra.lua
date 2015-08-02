
local win_linker = {
	{ "/NXCOMPAT /DYNAMICBASE";		Config = "win*" },
}

local win_common = {
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
		},
		{
			Name = "linux-gcc",
			DefaultOnHost = "linux",
			Tools = { "gcc" },
		},
		{
			Name = "win32-msvc",
			--DefaultOnHost = "windows",
			SupportedHosts = { "windows" },
			Inherit = win_common,
			--Tools = { {"msvc-vs2012"; TargetArch = "x86", WinSDK8 = "C:\\Program Files (x86)\\Windows Kits\\8.0"} },
			Tools = { {"msvc-vs2013"; TargetArch = "x86"} },
		},
		{
			Name = "win64-msvc",
			DefaultOnHost = "windows",
			Inherit = win_common,
			--Tools = { {"msvc-vs2012"; TargetArch = "x64", WinSDK8 = "C:\\Program Files (x86)\\Windows Kits\\8.0"} },
			Tools = { {"msvc-vs2013"; TargetArch = "x64"} },
		},
		{
			Name = "win64-mingw",
			SupportedHosts = { "windows" },
			Tools = { "mingw" },
			-- Link with the C++ compiler to get the C++ standard library.
			ReplaceEnv = {
    			PCHCOMPILE = "$(CC) $(_OS_CCOPTS) -c $(CPPDEFS:p-D) $(CPPPATH:f:p-I) $(CCOPTS) $(CCOPTS_$(CURRENT_VARIANT:u)) -o $(@) $(<)",
    			PCHCOMPILE_CXX = "$(CXX) $(_OS_CXXOPTS) -c $(CPPDEFS:p-D) $(CPPPATH:f:p-I) $(CXXOPTS) $(CXXOPTS_$(CURRENT_VARIANT:u)) -o $(@) $(<)",
				LD = "$(CXX)",
			},
		},
	},
}
