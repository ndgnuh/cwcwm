# C Plugin

DISCLAIMER: By writing or loading C plugin writer assume you already know what you're doing and
writer is not responsible for segfault, loss of data, kernel panic, bad API design,
thermonuclear war, or the current economic crisis. Always make sure the plugin you want to load
doesn't do anything malicious.

## Loading C Plugin

Loading C plugin can be done by using `cwc.plugin.load` API and to unload it using
`cwc.plugin.unload_byname`. If the plugin already exist `cwc.plugin.load` won't reload it,
if the plugin doesn't support unloading `unload_byname` is doing nothing.

## Writing C Plugin

You may be want to write C Plugin because you need full access to cwc internals and perhaps you
want to add amazing feature so that CwC become more awesome (pun intended). Knowledge of the CwC
internal code and wlroots may be necessary to start hacking.

The API for writing the C plugin is very similar to writing linux kernel module in fact some of the code
is from linux `module.h`, the difference is it's just start with `PLUGIN` instead of `MODULE`.

What it does under the hood is just loading the shared object using dlopen.
Let's take a look at `flayout.c` plugin for example.

```C
#include <wlr/types/wlr_output_layout.h>

#include "cwc/desktop/output.h"
#include "cwc/desktop/toplevel.h"
#include "cwc/layout/master.h"
#include "cwc/plugin.h"
#include "cwc/server.h"

// function arrange_flayout...

struct layout_interface fullscreen_impl = {.name    = "fullscreen",
                                           .arrange = arrange_flayout};

static int init()
{
    master_register_layout(&fullscreen_impl);

    return 0;
}

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
```

First we includes CwC header and other library header that we need. All the public CwC header
is commonly located at `/usr/include/cwc`, with that information you may need to adjust compiler flag.

```C
#include <wlr/types/wlr_output_layout.h>

#include "cwc/desktop/output.h"
#include "cwc/desktop/toplevel.h"
#include "cwc/layout/master.h"
#include "cwc/plugin.h"
#include "cwc/server.h"
```

Then create an entry point for the plugin load to call. The function name can be anything
as long as you call `plugin_init` macro with the function name as argument.
The function signature is `int (*name)(void)`, the return value is actually doesn't decide anything
but it'll follow linux convention with zero for success and non-zero for error in case
something change in the future.

```C
static int init_can_be_any_name()
{
    master_register_layout(&fullscreen_impl);

    return 0;
}

plugin_init(init_can_be_any_name);
```

If the plugin support unloading, `plugin_exit` macro can be used to mark a function to call
before the plugin is unloaded. If the plugin doesn't support unloading you may omit it.
Or if you support unloading but doesn't need a cleanup just pass an empty function.
The function signature is `void (*name)(void)`.

```C
static void fini()
{
    master_unregister_layout(&fullscreen_impl);
}

plugin_exit(fini);
```

Last is plugin metadata, `PLUGIN_NAME` and `PLUGIN_VERSION` is mandatory so that the loader can
manage it.

```C
PLUGIN_NAME("flayout");
PLUGIN_VERSION("0.1.0");
PLUGIN_DESCRIPTION("f layout we go f screen");
PLUGIN_LICENSE("MIT");
PLUGIN_AUTHOR("Dwi Asmoro Bangun <dwiaceromo@gmail.com>");
```

After writing the plugin now it's time for the compilation. The plugin need to be compiled
to shared object file (.so) with linker flag
`--allow-shlib-undefined`, `-shared`, and `--as-needed`

If you want to create a lua API in the plugin the convention used is `cwc.<plugin_name>`. For example
in `cwcle` the `PLUGIN_NAME` is `cwcle` so in the lua side it should be accessed with `cwc.cwcle`.
