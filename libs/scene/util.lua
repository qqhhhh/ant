local ecs = require "ecs"
local task = require "editor.task"
local asset = require "asset"

--local elog = require "editor.log"
--local db = require "debugger"

-- --windows dir
-- asset.insert_searchdir(1, "D:/Engine/ant/assets")
-- --mac dir
-- asset.insert_searchdir(2, "/Users/ejoy/Desktop/Engine/ant/assets")


local util = {}
util.__index = util

local world = nil

function util.start_new_world(input_queue, fbw, fbh, modules)
	if input_queue == nil then		
		log("input queue is not privided, no input event will be received!")
	end

	world = ecs.new_world {
		modules = modules,
		module_path = 'libs/?.lua;libs/?/?.lua',
		update_order = {"timesystem"},
		update_bydepend = true,
		args = { mq = input_queue, fb_size={w=fbw, h=fbh} },
    }
    
	task.loop(world.update)  
    return world
end

return util
