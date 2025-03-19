-- Marvin Build System Project Script
marvin_project "tool-reflector2"
	kind "ConsoleApp"
	debugdir "%{wks.location}/%{prj.name}"
	cppdialect "C++17"
	links {
		"external-pugixml"
	}
	debugargs {
		"--macros=MARVIN_API,EXPORT",
		"%{marvin_work_dir}/src/marvin-core/CApplication.h"
	}
	prebuildcommands {
		"{MKDIR} %{wks.location}/%{prj.name}/include"
	}
	
	--MCSSC = '"%{marvin_work_dir}/build/%{cfg.platform}/%{cfg.buildcfg}/tool-mcssc"'
	--MCSSC = iif(_OPTIONS["verbose"], MCSSC .. " --verbose", MCSSC)
