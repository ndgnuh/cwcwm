#include <assert.h>
#include <string.h>

#include "cwc/desktop/toplevel.h"
#include "cwc/plugin.h"
#include "cwc/signal.h"
#include "cwc/util.h"

static int call_counter    = 0;
static bool already_called = false;

static void on_toplevel_map(void *data)
{
    struct cwc_toplevel *toplevel = data;
    call_counter++;
    cwc_toplevel_set_fullscreen(toplevel, true);

    assert(toplevel);
    assert(call_counter == 1);
    assert(cwc_toplevel_is_fullscreen(toplevel));

    cwc_log(CWC_INFO, "C SIGNAL TEST OK");
}

static void on_custom_signal(void *data)
{
    assert(strcmp("testvalue", (char *)data) == 0);
    assert(!already_called);

    already_called = true;
    cwc_log(CWC_INFO, "C CUSTOM SIGNAL TEST OK");
}

static int signal_init()
{
    cwc_signal_connect("client::map", on_toplevel_map);
    cwc_signal_disconnect("client::map", on_toplevel_map);
    cwc_signal_connect("client::map", on_toplevel_map);

    cwc_signal_connect("custom::signal", on_custom_signal);
    cwc_signal_emit_c("custom::signal", "testvalue");
    cwc_signal_disconnect("custom::signal", on_custom_signal);
    cwc_signal_emit_c("custom::signal", "testvalue");

    return 0;
}

static void signal_test_eval()
{
    cwc_log(CWC_INFO, "UNLOADING C SIGNAL TEST PLUGIN OK");
}

plugin_init(signal_init);
plugin_exit(signal_test_eval);

PLUGIN_NAME("signal_test");
PLUGIN_VERSION("0.1.0");
PLUGIN_DESCRIPTION("signal test");
PLUGIN_LICENSE("0BSD");
PLUGIN_AUTHOR("Antoni Morbell");
