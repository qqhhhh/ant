local ecs = ...
local world = ecs.world
local w = world.w

local math3d = require "math3d"

local b = ecs.system "bounding_system"

local function init_bounding(bounding, bb)
    if bb then
        bounding.aabb = math3d.mark(bb.aabb)
        bounding.scene_aabb = math3d.mark(math3d.aabb())
    end
end

function b:entity_init()
	for e in w:select "INIT bounding:update mesh?in simplemesh?in" do
		local m = e.mesh or e.simplemesh
		if m then
			init_bounding(e.bounding, m.bounding)
		end
	end
end

local s = ecs.system "scenespace_system"
function s:entity_init()
	for v in w:select "INIT scene scene_needchange?out" do
		v.scene_needchange = true
	end
end

local sceneupdate_sys = ecs.system "scene_update_system"
function sceneupdate_sys:init()
	ecs.group(0):enable "scene_update"
	ecs.group_flush()
end

local g_sys = ecs.system "group_system"
function g_sys:start_frame()
	ecs.group_flush()
end

local static_sceneobj = ecs.system "standalone_scene_object_system"
function static_sceneobj:entity_init()
	for e in w:select "INIT standalone_scene_object scene scene_update?out standalone_scene_object_update?out" do
		e.scene_update = true
		e.standalone_scene_object_update = true
	end
end

function static_sceneobj:end_frame()
	for e in w:select "standalone_scene_object_update standalone_scene_object scene_update?out" do
		e.scene_update = nil
	end
	w:clear "standalone_scene_object_update"
end