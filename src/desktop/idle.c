/* idle.c - define idle behavior
 *
 * Copyright (C) 2024 Dwi Asmoro Bangun <dwiaceromo@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Current implementation will inhibit when any inhibitor exist whether the
 * toplevel visible or not. TODO: inhibit when the inhibitor visible
 */

#include <stdlib.h>
#include <wayland-util.h>

#include "cwc/desktop/idle.h"
#include "cwc/server.h"

static void on_destroy_inhibitor(struct wl_listener *listener, void *data)
{
    struct cwc_idle_inhibitor *inhibitor =
        wl_container_of(listener, inhibitor, destroy_inhibitor_l);

    wl_list_remove(&inhibitor->destroy_inhibitor_l.link);
    wl_list_remove(&inhibitor->link);

    if (wl_list_empty(&inhibitor->cwc_idle->inhibitors))
        wlr_idle_notifier_v1_set_inhibited(inhibitor->cwc_idle->idle_notifier,
                                           false);

    free(inhibitor);
}

static void on_new_inhibitor(struct wl_listener *listener, void *data)
{
    struct cwc_idle *idle = wl_container_of(listener, idle, new_inhibitor_l);
    struct wlr_idle_inhibitor_v1 *wlr_inhibitor = data;

    struct cwc_idle_inhibitor *inhibitor  = calloc(1, sizeof(*inhibitor));
    inhibitor->wlr_inhibitor              = wlr_inhibitor;
    inhibitor->cwc_idle                   = idle;
    inhibitor->destroy_inhibitor_l.notify = on_destroy_inhibitor;
    wl_signal_add(&wlr_inhibitor->events.destroy,
                  &inhibitor->destroy_inhibitor_l);

    wlr_idle_notifier_v1_set_inhibited(idle->idle_notifier, true);
    wl_list_insert(&idle->inhibitors, &inhibitor->link);
}

void cwc_idle_init(struct cwc_server *s)
{
    struct cwc_idle *idle = calloc(1, sizeof(*idle));
    s->idle               = idle;
    idle->server          = s;
    wl_list_init(&idle->inhibitors);

    idle->inhibit_manager        = wlr_idle_inhibit_v1_create(s->wl_display);
    idle->idle_notifier          = wlr_idle_notifier_v1_create(s->wl_display);
    idle->new_inhibitor_l.notify = on_new_inhibitor;
    wl_signal_add(&idle->inhibit_manager->events.new_inhibitor,
                  &idle->new_inhibitor_l);
}

void cwc_idle_fini(struct cwc_server *s)
{
    free(s->idle);
    s->idle = NULL;
}
