project "raven_serialize"
	kind "ConsoleApp"
	language "C++"
	location(_ACTION)

	files { "../**.cpp", "../**.hpp" }
	includedirs
	{
		"../src",
	}

	targetdir(EngineRootLocation.."/bin")
	targetname "raven_serialize"
	
	configureWindowsSDK()

	configuration "Debug"
		defines { "DEBUG" }
		symbols "on"
		optimize "Off"
		targetsuffix "_d"
		objdir(_ACTION.."/obj/Debug")

	configuration "Release"
		optimize "Full"
		objdir(_ACTION.."/obj/Release")
