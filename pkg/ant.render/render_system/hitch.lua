local ecs   = ...
local world = ecs.world
local w     = world.w

local math3d    = require "math3d"

local h = ecs.component "hitch"
function h.init(hh)
    assert(hh.group ~= nil)
    hh.visible_masks = 0
    hh.cull_masks = 0
    return hh
end

local hitch_sys = ecs.system "hitch_system"

function hitch_sys:entity_init()
    for e in w:select "INIT hitch hitch_bounding?out" do
        e.hitch_bounding = true
    end
end

function hitch_sys:entity_ready()
    local groups = {}
    for e in w:select "hitch_bounding hitch:in eid:in" do
        local g = groups[e.hitch.group]
        if g == nil then
            g = {}
            groups[e.hitch.group] = g
        end
        g[#g+1] = e.eid
    end

    for gid, hitchs in pairs(groups) do
        local g = ecs.group(gid)

        g:enable "hitch_tag"
        ecs.group_flush "hitch_tag"

        local h_aabb = math3d.aabb()
        for re in w:select "hitch_tag bounding:in" do
            h_aabb = math3d.aabb_merge(h_aabb, re.bounding.aabb)
        end

        if math3d.aabb_isvalid(h_aabb) then
            for _, heid in ipairs(hitchs) do
                local e<close> = w:entity(heid, "bounding:update scene_needchange?out")
                math3d.unmark(e.bounding.aabb)
                e.bounding.aabb = math3d.mark(h_aabb)
            end
        end
    end

    w:clear "hitch_bounding"
end