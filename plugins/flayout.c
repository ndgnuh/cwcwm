#include <wlr/types/wlr_output_layout.h>

#include "cwc/desktop/output.h"
#include "cwc/desktop/toplevel.h"
#include "cwc/layout/master.h"
#include "cwc/plugin.h"
#include "cwc/server.h"

static void arrange_flayout(struct cwc_toplevel **toplevels,
                            int len,
                            struct cwc_output *output,
                            struct master_state *master_state)
{
    int i                         = 0;
    struct cwc_toplevel *toplevel = toplevels[i];
    double lx, ly;
    wlr_output_layout_output_coords(server.output_layout,
                                    toplevel->container->output->wlr_output,
                                    &lx, &ly);
    while (toplevel) {
        cwc_toplevel_set_position(toplevel, lx, ly);
        cwc_toplevel_set_size_surface(toplevel, output->wlr_output->width,
                                      output->wlr_output->height);

        toplevel = toplevels[++i];
    }
}

struct layout_interface fullscreen_impl = {.name    = "fullscreen",
                                           .arrange = arrange_flayout};

static int init()
{
    master_register_layout(&fullscreen_impl);

    return 0;
}

/* crash when removing layout while its the current layout
 * TODO: refactor layout list to array instead of linked list.
 */
static void fini()
{
    master_unregister_layout(&fullscreen_impl);
}

plugin_init(init);
plugin_exit(fini);

PLUGIN_NAME("flayout");
PLUGIN_VERSION("0.1.0");
PLUGIN_DESCRIPTION("f layout we go f screen");
PLUGIN_LICENSE("MIT");
PLUGIN_AUTHOR("Dwi Asmoro Bangun <dwiaceromo@gmail.com>");
