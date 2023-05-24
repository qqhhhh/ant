local ltask     = require "ltask"

local math3d    = require "math3d"

local EFK_SERVER<const> = ltask.queryservice "ant.efk|efk"

local handle_mt = {
    is_alive = function(self)
        ltask.fork(function ()
            self.alive = ltask.call(EFK_SERVER, "is_alive", self.handle)
        end)
        return self.alive
    end,
    set_stop = function(self, delay)
        ltask.send(EFK_SERVER, "set_stop", self.handle, delay)
    end,
    set_transform = function(self, mat)
        ltask.send(EFK_SERVER, "set_transform", self.handle, math3d.serialize(mat))
    end,
    set_time = function(self, time)
        ltask.send(EFK_SERVER, "set_time", self.handle, time)
    end,
    set_pause = function(self, p)
        assert(p ~= nil)
        ltask.send(EFK_SERVER, "set_pause", self.handle, p)
    end,
    
    set_speed = function(self, speed)
        assert(speed ~= nil)
        ltask.send(EFK_SERVER, "set_speed", self.handle, speed)
    end,
    
    set_visible = function(self, v)
        assert(v ~= nil)
        ltask.send(EFK_SERVER, "set_visible", self.handle, v)
    end,
}

local function create(efk_handle, mat, speed)
    local h = setmetatable({
        alive       = true,
        handle      = ltask.call(EFK_SERVER, "play", efk_handle, math3d.value_ptr(mat), speed),
    }, {__index = handle_mt})
    return h
end

return {
    create      = create,
}