#ifndef _CWC_MASTER_H
#define _CWC_MASTER_H

#include "cwc/types.h"

struct cwc_toplevel;
struct cwc_output;

struct master_state;

struct layout_interface {
    char *name;

    /* toplevels doesn't include maximized/fullscreen or not tiled client, so
     * the implementation can focus arranging the given toplevels.
     */
    void (*arrange)(struct cwc_toplevel **toplevels,
                    int len,
                    struct cwc_output *output,
                    struct master_state *master_state);

    // private

    // circular, intrusive, no head for easier navigation.
    struct layout_interface *next;
    struct layout_interface *prev;
};

void master_register_layout(struct layout_interface *impl);
void master_unregister_layout(struct layout_interface *impl);

struct layout_interface *get_default_master_layout();

void master_arrange_update(struct cwc_output *output);

struct cwc_toplevel *master_get_master(struct cwc_output *output);

void master_set_master(struct cwc_toplevel *toplevel);

#endif // !_CWC_MASTER_H