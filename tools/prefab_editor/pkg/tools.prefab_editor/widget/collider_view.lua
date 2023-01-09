local ecs = ...
local world = ecs.world
local w = world.w
ecs.require "widget.base_view"
local iom           = ecs.import.interface "ant.objcontroller|iobj_motion"
local imaterial = ecs.import.interface "ant.asset|imaterial"
local prefab_mgr    = ecs.require "prefab_manager"
local anim_view     = ecs.require "widget.animation_view"
local imgui     = require "imgui"
local utils     = require "common.utils"
local math3d    = require "math3d"
local uiproperty = require "widget.uiproperty"
local hierarchy     = require "hierarchy_edit"
local BaseView      = require "widget.view_class".BaseView
local ColliderView  = require "widget.view_class".ColliderView
local collider_type = {"sphere", "box", "capsule"}

function ColliderView:_init()
    BaseView._init(self)
    self.radius     = uiproperty.Float({label = "Radius", min = 0.01, max = 10.0, speed = 0.01}, {})
    self.height     = uiproperty.Float({label = "Height", min = 0.01, max = 10.0, speed = 0.01}, {})
    self.half_size  = uiproperty.Float({label = "HalfSize", min = 0.01, max = 10.0, speed = 0.01, dim = 3}, {})
    self.color      = uiproperty.Color({label = "Color", dim = 4})
end

function ColliderView:set_model(eid)
    if not BaseView.set_model(self, eid) then return false end

    local tp = hierarchy:get_template(eid)
    local e <close> = w:entity(eid, "collider:in")
    local collider = e.collider
    if collider.sphere then
        self.radius:set_getter(function()
            local scale = math3d.totable(iom.get_scale(eid))
            return scale[1] / 100
        end)
        self.radius:set_setter(function(r)
            local e <close> = w:entity(eid)
            iom.set_scale(e, r * 100)
            --prefab_mgr:update_current_aabb(self.e)
            world:pub {"UpdateAABB", self.eid}
            anim_view.record_collision(self.eid)
        end)
        
    elseif collider.capsule then
        self.radius:set_getter(function() return world[eid].collider.capsule[1].radius end)
        self.radius:set_setter(function(r) end)
        self.height:set_getter(function() return world[eid].collider.capsule[1].height end)
        self.height:set_setter(function(h) end)
    elseif collider.box then
        self.half_size:set_getter(function()
            local scale = math3d.totable(iom.get_scale(eid))
            return {scale[1] / 200, scale[2] / 200, scale[3] / 200}
        end)
        self.half_size:set_setter(function(sz)
            local e <close> = w:entity(self.eid)
            iom.set_scale(e, {sz[1] * 200, sz[2] * 200, sz[3] * 200})
            --prefab_mgr:update_current_aabb(self.e)
            world:pub {"UpdateAABB", self.eid}
            anim_view.record_collision(self.eid)
        end)
    end
    self.color:set_getter(function() return self:on_get_color() end)
    self.color:set_setter(function(...) self:on_set_color(...) end)
    self:update()
    return true
end

function ColliderView:has_scale()
    return false
end

function ColliderView:on_set_color(...)
    local e <close> = w:entity(self.eid)
    imaterial.set_property(e, "u_color", ...)
end

function ColliderView:on_get_color()
    log.warn("should not call imaterial.get_property, matieral properties should edit from material view")
    -- local color = math3d.totable(rc.value)
    return {0.0, 0.0, 0.0, 0.0}--{color[1], color[2], color[3], color[4]}
end

function ColliderView:update()
    BaseView.update(self)
    local e <close> = w:entity(self.eid, "collider:in")
    if e.collider.sphere then
        self.radius:update()
    elseif e.collider.capsule then
        self.radius:update()
        self.height:update()
    elseif e.collider.box then
        self.half_size:update()
    end
    self.color:update()
end

function ColliderView:show()
    BaseView.show(self)
    local e <close> = w:entity(self.eid, "collider:in")
    if e.collider.sphere then
        self.radius:show()
    elseif e.collider.capsule then
        self.radius:show()
        self.height:show()
    elseif e.collider.box then
        self.half_size:show()
    end
    local slot_list = hierarchy.slot_list
    if slot_list then
        imgui.widget.PropertyLabel("LinkSlot")
        if imgui.widget.BeginCombo("##LinkSlot", {e.slot_name or "None", flags = imgui.flags.Combo {}}) then
            for name, eid in pairs(slot_list) do
                if imgui.widget.Selectable(name, e.slot_name and e.slot_name == name) then
                    e.slot_name = name
                    world:pub {"EntityEvent", "parent", self.eid, eid}
                end
            end
            imgui.widget.EndCombo()
        end
    end
    self.color:show()
end

return ColliderView