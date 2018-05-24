local log = log and log(...) or print

local treecontrol = require "editor.tree"
local eu = require "editor.util"

local hierarchyview = {}

local tree = treecontrol.new()
tree.view.hidebuttons ="YES"
tree.view.hidelines   ="YES"
tree.view.title 		= "World"

hierarchyview.window = tree

function tree.view:selection_cb(id, status)
	local cb = hierarchyview.selection_cb
	if cb then
		cb(hierarchyview, id, status)
	end
end

function hierarchyview:build(htree, ud_table)	
	local treeview = self.window
	local function constrouct_treeview(tr, parent)
		for k, v in pairs(tr) do
			local ktype = type(k)
			if ktype == "string" or ktype == "number" then
				local vtype = type(v)
				local function add_child(parent, name)
					local child = treeview:add_child(parent, name)
					local eid = assert(ud_table[name])
					child.eid = eid
					return child
				end
				
				if vtype == "table" then
					local child = add_child(parent, k)
					constrouct_treeview(v, child)
				elseif vtype == "string" then
					add_child(parent, v)
				end
			else
				log("not support ktype : ", ktype)
			end
	
		end
	end
	
	constrouct_treeview(htree, nil)
end

return hierarchyview