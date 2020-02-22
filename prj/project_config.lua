local rv_serialize_path = EngineRootLocation.."/framework/core/raven_serialize"
local rv_serialize_src = rv_serialize_path.."/src"

local module_definition =
{
	["name"] = "raven_serialize",
	["kind"] = "shared_lib",
	["prj_location"] = rv_serialize_path.."/prj/".._ACTION,
	["dependencies"] = { "jsoncpp" },
	["files"] = { rv_serialize_src.."/**.cpp", rv_serialize_src.."/**.hpp" },
	["include_dirs"] = { rv_serialize_src },
	["defines"] = {},
	["excludes"] = { rv_serialize_src.."/examples/**", rv_serialize_src.."/jsoncpp/**" },
	["link_type"] = "dynamic",
}

registerModuleDef(module_definition)
