project "raven_serialize"
	if SerializationStandalone then
		kind "ConsoleApp"
	else
		kind "SharedLib"
		defines { "RAVEN_SERIALIZE_EXPORTS" }
	end
	language "C++"
	location(_ACTION)

	files { "../**.cpp", "../**.hpp" }
	includedirs
	{
		"../src",
	}
	
	if not SerializationStandalone then
		excludes { "../src/examples/**", "../src/jsoncpp/**" }
		links { "jsoncpp" }
		includedirs { EngineRootLocation.."/framework/core/jsoncpp/include", }
	end

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
