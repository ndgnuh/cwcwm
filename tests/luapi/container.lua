local cwc = cwc

local function property_test(cont, c)
    assert(type(cont.clients) == "table")
    assert(string.find(tostring(cont.clients[1]), "cwc_client"))
    assert(cont.front == c)

    local geom = cont.geometry
    assert(type(cont.geometry) == "table")
    assert(type(geom) == "table")
    assert(type(geom.x) == "number")
    assert(type(geom.y) == "number")
    assert(type(geom.width) == "number")
    assert(type(geom.height) == "number")

    print(cont.insert_mark)
    assert(cont.insert_mark == false)
    cont.insert_mark = true
    assert(cont.insert_mark == true)
end

local function method_test(cont)
    cont:focusidx(1)
    cont:swap(cont)
    assert(#cont.client_stack == #cont:get_client_stack(true))
end

local function test()
    local c = cwc.client.focused()
    local cont = c.container

    property_test(cont, c)
    method_test(cont)

    print("cwc_container test PASSED")
end

return test
