local cwc = cwc

local function test()
    cwc.plugin.load("./build/tests/csignal_test.so")
    cwc.spawn_with_shell("kitty")
end

return test
