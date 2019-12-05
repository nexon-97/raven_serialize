include "build_config.lua"

function latestSDK10Version()
	local arch = iif(os.is64bit(), "\\WOW6432Node\\", "\\")
	local version = os.getWindowsRegistry("HKLM:SOFTWARE" .. arch .."Microsoft\\Microsoft SDKs\\Windows\\v10.0\\ProductVersion")
	return iif(version ~= nil, version .. ".0", nil)
end

function configureWindowsSDK()
	local sdkVersion = latestSDK10Version()
	if sdkVersion ~= nil then
		systemversion(sdkVersion)
	end
end

SerializationStandalone = true
EngineRootLocation = path.getabsolute("../")

workspace "raven_serialize"
	configurations { "Debug", "Development", "Release" }
	platforms { "x64" }
	cppdialect("c++17")

solution "raven_serialize"
	location ("../.")

	include ("./project_config.lua")
