
local mingw_common = {
	Env = {
		CCOPTS_RELEASE = {
			{ "-O2" },
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
}
