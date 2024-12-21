-------------------------------------------------------------------------------
--- Useful functions for client manipulation.
--
-- @author Dwi Asmoro Bangun
-- @copyright 2024
-- @license GPLv3
-- @module cuteful.client
-------------------------------------------------------------------------------

local cwc = cwc

local client = {}

--- Get a client by its relative index.
--
-- @staticfct getidx
-- @tparam number idx Index relative to the this client
-- @tparam[opt] cwc_client client Base client (index 0).
-- @treturn cwc_client
function client.getidx(idx, _client)
    local c = _client or cwc.client.focused()
    local containers = c.screen:get_containers(true)
    if (#containers <= 1) then return end

    -- find the client index
    local hop_distance = idx % #containers
    local focused_idx = 0
    for i = 1, #containers do
        if c.container == containers[i] then
            focused_idx = i - 1
            break
        end
    end
    local nextidx = (focused_idx + hop_distance) % #containers
    return containers[nextidx + 1].front
end

--- Focus a client by its relative index.
--
-- @staticfct cuteful.client.focusidx
-- @tparam number idx Index relative to the this client
-- @tparam[opt] cwc_client client Base client (index 0).
-- @noreturn
function client.focusidx(idx, _client)
    local c = client.getidx(idx, _client)
    if c then c:focus() end
end

--- Swap a client by its relative index.
--
-- @staticfct swapidx
-- @tparam number idx Index relative to the this client
-- @tparam[opt] cwc_client client Base client (index 0).
-- @noreturn
function client.swapidx(idx, _client)
    _client = _client or cwc.client.focused()
    local c = client.getidx(idx, _client)
    if c then c.container:swap(_client.container) end
end

--- Maximize client horizontally.
--
-- @staticfct maximize_horizontal
-- @tparam cwc_client client The client
-- @noreturn
function client.maximize_horizontal(_c)
    local c = _c or cwc.client.focused()

    if c.maximized then
        c.maximized = false
        return
    end

    local workarea = c.screen.workarea
    local new_geom = c.geometry
    new_geom.x = workarea.x
    new_geom.width = workarea.width

    c.maximized = true
    c.geometry = new_geom
end

--- Maximize client vertically.
--
-- @staticfct maximize_vertical
-- @tparam cwc_client client The client
-- @noreturn
function client.maximize_vertical(_c)
    local c = _c or cwc.client.focused()

    if c.maximized then
        c.maximized = false
        return
    end

    local workarea = c.screen.workarea
    local new_geom = c.geometry
    new_geom.y = workarea.y
    new_geom.height = workarea.height
    c.maximized = true
    c.geometry = new_geom
end

--- Restore (=unminimize) a client in the screen.
--
-- @staticfct restore
-- @tparam boolean active_tag Unminimize client in the active tag only.
-- @tparam cwc_screen s The screen to use.
-- @treturn cwc_client The restored client.
function client.restore(active_tag, s)
    s = s or cwc.screen.focused()
    local cls = s:get_minimized(active_tag)
    local c = cls[1]
    if c then
        c.minimized = false
        return c
    end
end

--- Get the master client.
--
-- @staticfct get_master
-- @tparam cwc_screen s Screen to use
-- @treturn cwc_client The master client
function client.get_master(s)
    s = s or cwc.screen.focused()
    local cts = s:get_containers(true)
    return cts[1].front
end

--- Set the master client or container.
--
-- @staticfct set_master
-- @tparam cwc_client c New master client.
-- @tparam[opt=false] boolean swap_container Whether to swap the container.
-- @noreturn
function client.set_master(c, swap_container)
    local master = client.get_master()
    if swap_container then
        c.container:swap(master.container)
    else
        c:swap(master)
    end
end

return client
