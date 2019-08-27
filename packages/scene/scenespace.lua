local ecs = ...
local world = ecs.world

ecs.import "ant.event"

local mathpkg 	= import_package "ant.math"
local ms 		= mathpkg.stack

local assetpkg 	= import_package "ant.asset"
local assetmgr 	= assetpkg.mgr

local animodule = require "hierarchy.animation"

ecs.component_alias("attach", "entityid")
ecs.component_alias("ignore_parent_scale", "boolean") {depend = "hierarchy"}

ecs.singleton "hierarchy_transform_result"

local ur = ecs.singleton "hierarchy_update_result"
local function reset_hierarchy_update_result(rr)
	rr.removed_eids 	= {}
	rr.remove_trees 	= {}
end

function ur.init()
	local rr = {}
	reset_hierarchy_update_result(rr)
	return rr
end

local scene_space = ecs.system "scene_space"
scene_space.dependby "primitive_filter_system"
scene_space.dependby "remove_hierarchy_system"

scene_space.singleton "event"
scene_space.singleton "hierarchy_transform_result"
scene_space.singleton "hierarchy_update_result"

local pseudoroot_eid = -1

local mark_mt = { 
	__index = function(marked, eid) 
		if eid == pseudoroot_eid then
			marked[pseudoroot_eid] = false
			return false
		end
		local e = world[eid]
		
		local pid = e.transform.parent
		if pid and marked[pid] then
			marked[eid] = pid
			return true
		end

		marked[eid] = false
		return false
	end 
}

-- local render_mark_mt = {
-- 	__index = function(marked, eid) 
-- 		if eid == pseudoroot_eid then
-- 			marked[pseudoroot_eid] = false
-- 			return false
-- 		end
-- 		local e = world[eid]
-- 		if e.hierarchy == nil then
-- 			local pid = e.transform.parent
-- 			if pid and marked[pid] then
-- 				marked[eid] = pid
-- 				return true
-- 			end
-- 		end

-- 		marked[eid] = false
-- 		return false
-- 	end
-- }

local function mark_tree(tree, componenttype)
	for _, eid in world:each(componenttype) do
		assert(world[eid].transform)
		local _ = tree[eid]
	end
end

local function tree_sort(tree)
	local r = {}

	local from = 1
	local to = 1
	-- find leaves
	local leaf = {}
	for eid, pid in pairs(tree) do
		if pid then
			if leaf[pid] then	-- fake leaf
				-- todo remove result
				for i = 1, to-1 do
					if r[i] == pid then
						to = to - 1
						r[i] = r[to]
						break
					end
				end
			end
			leaf[pid] = false
			if leaf[eid] == nil then
				leaf[eid] = true	-- mark leaf
				r[to] = eid
				to = to + 1
			end
		end
	end
	while to > from do
		local lb = from
		from = to
		for i = lb, to-1 do
			local pid = tree[r[i]]
			if pid and tree[pid] and not leaf[pid] then
				r[to] = pid
				to = to + 1
				leaf[pid] = true
			end
		end
	end
	return r
end

local function update_hirarchy_entity_world(trans, ignore_parentscale)
	local srt = ms:srtmat(trans)
	local peid = trans.parent
	if peid then
		local parent = world[peid]
		local pt = parent.transform

		local finalmat = ms:mul_srtmat(pt.world, srt, ignore_parentscale)
		ms(trans.world, finalmat, "=")
	else
		ms(trans.world, srt, "=")
	end
	return trans.world
end

local function fetch_sort_tree_result(tree, componenttype)
	setmetatable(tree, mark_mt)
	mark_tree(tree, componenttype)
	return tree_sort(tree)
end

local function fetch_hirarchy_tree(tree)
	return fetch_sort_tree_result(tree, "hierarchy")
end

local function mark_cache(eid, cache_result)
	local e = world[eid]
	local t = e.transform
	
	local cachemat = update_hirarchy_entity_world(t, e.ignore_parent_scale)
	assert(type(cachemat) == 'userdata')

	local hiecomp = assert(e.hierarchy)
	if hiecomp.ref_path then
		local hiehandle = assetmgr.get_resource(hiecomp.ref_path).handle
		if t.hierarchy_result == nil then
			local bpresult = animodule.new_bind_pose_result(#hiehandle)
			hiehandle:bindpose_result(bpresult)
			t.hierarchy_result = setmetatable({}, {__index=function(t, key)
				local jointidx = hiehandle:joint_index(key)
				local j = bpresult:joint(jointidx)
				t[key] = j
				return j
			end})
		end
	end
	cache_result[eid] = {world=cachemat, hierarchy=t.hierarchy_result}
end

local function update_hierarchy_tree(tree, cache_result)
	if next(tree) then
		local sort_result = fetch_hirarchy_tree(tree)
		for i = #sort_result, 1, -1 do
			local eid = sort_result[i]
			mark_cache(eid, cache_result)
		end
	end
end

local function update_transform_field(trans, events, init)
	if init then
		assert(events == nil or (not next(events)))
		return true
	end

	local changed
	for k, v in pairs(events) do
		changed = true
		if k == 's' or k == 'r' or k == 't' then
			ms(trans[k], v, "=")
		elseif k == 'parent' then
			trans.parent = v
		else
			changed = changed or nil
		end
	end

	return changed
end

function scene_space:delete()
	local hierarchy_cache = self.hierarchy_transform_result	
	local removed_eids = {}
	for eid, result in world:each_removed("hierarchy", true) do
		hierarchy_cache[eid] = nil
		removed_eids[eid] = result
	end

	if next(removed_eids) then
		local trees = {}
		for _, eid in world:each "transform" do
			assert(removed_eids[eid] == nil)

			local e = world[eid]
			local peid = e.transform.parent
			if removed_eids[peid] then
				local subtree = trees[peid]
				if subtree == nil then
					subtree = {}
					trees[peid] = subtree
				end
				subtree[#subtree+1] = eid
			end
		end

		self.hierarchy_update_result.remove_trees = trees
		self.hierarchy_update_result.removed_eids = removed_eids
		world:update_func "handle_removed_hierarchy" ()
	end
end

local function add_hierarchy_tree_item(eid, events, init, trees)
	local trans = world[eid].transform
	local oldparent = trans.parent
	if events then
		update_transform_field(trans, events, init)
	end

	local newparent = trans.parent
	if newparent ~= oldparent then
		local parentparent = world[newparent].transform.parent
		trees[newparent] = parentparent
	end
	
	trees[eid] = newparent or pseudoroot_eid
end

function scene_space:post_init()
	for eid in world:each_new "transform" do
		self.event:new(eid, "transform")
	end
end

local need_check_components_changed = {"hierarchy", "ignore_parent_scale"}

function scene_space:event_changed()
	local trees = {}
	
	for eid, events, init in self.event:each "transform" do
		local e = world[eid]
		if e.hierarchy then
			add_hierarchy_tree_item(eid, events, init, trees)
		else
			update_transform_field(e.transform, events, init)
			local trans = e.transform
			--TODO: mark parent to cache, if no other hirarchy node change, we can only call 'mark_cache' function here
			local peid = trans.parent
			if peid then
				assert(world[peid].hierarchy)
				add_hierarchy_tree_item(peid, nil, true, trees)
			end
		end
	end

	for i=1, #need_check_components_changed do
		local compname = need_check_components_changed[i]
		for eid in world:each_new(compname) do
			add_hierarchy_tree_item(eid, nil, true, trees)
		end
	end

	-- remove 'ignore_parent_scale' need update hierarchy tree
	for eid in world:each_removed "ignore_parent_scale" do
		if world[eid] then
			add_hierarchy_tree_item(eid, nil, true, trees)
		end
	end

	if next(trees) then
		update_hierarchy_tree(trees, self.hierarchy_update_result)
	end
end

local remove_hierarchy_system = ecs.system "remove_hierarchy_system"
remove_hierarchy_system.singleton "hierarchy_update_result"
remove_hierarchy_system.singleton "hierarchy_transform_result"

function remove_hierarchy_system:handle_removed_hierarchy()
	local updateresult 		= self.hierarchy_update_result
	local remove_trees 		= updateresult.remove_trees

	local removeeids 		= self.hierarchy_update_result.removed_eids

	-- move removed hirarchy entity transform to children
	for hie_eid, remove_result in pairs(removeeids) do
		local hie_entity = remove_result[2]

		local subtree = remove_trees[hie_eid]
		if subtree then
			assert(hie_entity.transform, "remove 'hierarchy' component should not remove transform component at the mean time")
			local hie_srt = ms:srtmat(hie_entity.transform)
			for _, subeid in ipairs(subtree) do
				local subentity = assert(world[subeid])
				local trans = subentity.transform

				assert(trans.parent == hie_eid)
				trans.parent = nil

				local localsrt = ms:srtmat(trans)
				ms(trans.world, hie_srt, localsrt, "*=")

				local s, r, t = ms(trans.world, "~PPP")
				ms(trans.s, s, "=", trans.r, r, "=", trans.t, t, "=")
			end
		end
	end

	reset_hierarchy_update_result(self.hierarchy_update_result)
end