/* output.c - output/screen management
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_alpha_modifier_v1.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_output_management_v1.h>
#include <wlr/types/wlr_output_power_management_v1.h>
#include <wlr/types/wlr_scene.h>

#include "cwc/config.h"
#include "cwc/desktop/layer_shell.h"
#include "cwc/desktop/output.h"
#include "cwc/desktop/toplevel.h"
#include "cwc/input/seat.h"
#include "cwc/layout/bsp.h"
#include "cwc/layout/container.h"
#include "cwc/layout/master.h"
#include "cwc/luaclass.h"
#include "cwc/luaobject.h"
#include "cwc/server.h"
#include "cwc/signal.h"
#include "cwc/types.h"
#include "cwc/util.h"

static inline void insert_tiled_toplevel_to_bsp_tree(struct cwc_output *output,
                                                     int view)
{
    struct cwc_container *container;
    wl_list_for_each(container, &output->state->containers,
                     link_output_container)
    {
        if (!cwc_container_is_visible_in_workspace(container, view)
            || cwc_container_is_floating(container)
            || container->bsp_node != NULL)
            continue;

        bsp_insert_container(container, view);
        if (cwc_container_is_maximized(container)
            || cwc_container_is_fullscreen(container))
            bsp_node_disable(container->bsp_node);
    }
}

void cwc_output_tiling_layout_update(struct cwc_output *output, int view)
{
    enum cwc_layout_mode mode =
        cwc_output_get_current_view_info(output)->layout_mode;

    view = view ? view : output->state->active_workspace;

    switch (mode) {
    case CWC_LAYOUT_BSP:
        bsp_update_root(output, view);
        break;
    case CWC_LAYOUT_MASTER:
        master_arrange_update(output);
        break;
    default:
        break;
    }
}

static struct cwc_output_state *cwc_output_state_create()
{
    struct cwc_output_state *state = calloc(1, sizeof(struct cwc_output_state));

    state->active_tag            = 1;
    state->active_workspace      = 1;
    state->max_general_workspace = 9;
    wl_list_init(&state->focus_stack);
    wl_list_init(&state->toplevels);
    wl_list_init(&state->containers);
    wl_list_init(&state->minimized);

    for (int i = 0; i < MAX_WORKSPACE; i++) {

        state->view_info[i].useless_gaps              = g_config.useless_gaps;
        state->view_info[i].layout_mode               = CWC_LAYOUT_FLOATING;
        state->view_info[i].master_state.master_count = 1;
        state->view_info[i].master_state.column_count = 1;
        state->view_info[i].master_state.mwfact       = 0.5;
        state->view_info[i].master_state.current_layout =
            get_default_master_layout();
    }

    return state;
}

static inline void cwc_output_state_save(struct cwc_output *output)
{
    // TODO: move to fallback output or not needed?
    cwc_hhmap_insert(server.output_state_cache, output->wlr_output->name,
                     output->state);
}

/* return true if restored, false otherwise */
static bool cwc_output_state_try_restore(struct cwc_output *output)
{
    output->state =
        cwc_hhmap_get(server.output_state_cache, output->wlr_output->name);

    if (!output->state)
        return false;

    struct cwc_output *old_output = output->state->old_output;
    struct cwc_container *container;

    // update the output in the container to the new one
    wl_list_for_each(container, &server.containers, link)
    {
        if (container->output == old_output)
            container->output = output;
    }

    // update for the layer shell
    struct cwc_layer_surface *layer_surface;
    wl_list_for_each(layer_surface, &server.layer_shells, link)
    {
        if (layer_surface->output == old_output) {
            layer_surface->output                    = output;
            layer_surface->wlr_layer_surface->output = output->wlr_output;
        }
    }

    cwc_hhmap_remove(server.output_state_cache, output->wlr_output->name);
    return true;
}

/* actually won't be destroyed since how do I know when the output will
 * come back???
 */
static inline void cwc_output_state_destroy(struct cwc_output_state *state)
{
    free(state);
}

static void _output_configure_scene(struct cwc_output *output,
                                    struct wlr_scene_node *node,
                                    float opacity)
{
    if (node->data) {
        struct cwc_container *container =
            cwc_container_try_from_data_descriptor(node->data);
        if (container)
            opacity = container->opacity;
    }

    if (node->type == WLR_SCENE_NODE_BUFFER) {
        struct wlr_scene_buffer *buffer = wlr_scene_buffer_from_node(node);
        struct wlr_scene_surface *surface =
            wlr_scene_surface_try_from_buffer(buffer);

        if (surface) {
            const struct wlr_alpha_modifier_surface_v1_state
                *alpha_modifier_state =
                    wlr_alpha_modifier_v1_get_surface_state(surface->surface);
            if (alpha_modifier_state != NULL) {
                opacity *= (float)alpha_modifier_state->multiplier;
            }
        }

        wlr_scene_buffer_set_opacity(buffer, opacity);
    } else if (node->type == WLR_SCENE_NODE_TREE) {
        struct wlr_scene_tree *tree = wlr_scene_tree_from_node(node);
        struct wlr_scene_node *node;
        wl_list_for_each(node, &tree->children, link)
        {
            _output_configure_scene(output, node, opacity);
        }
    }
}

static void output_repaint(struct cwc_output *output)
{
    _output_configure_scene(output, &server.scene->tree.node, 1.0f);
}

static void on_output_frame(struct wl_listener *listener, void *data)
{
    struct cwc_output *output = wl_container_of(listener, output, frame_l);
    struct wlr_scene *scene   = server.scene;
    struct wlr_scene_output *scene_output =
        wlr_scene_get_scene_output(scene, output->wlr_output);
    struct timespec now;

    if (!scene_output)
        return;

    output_repaint(output);

    /* Render the scene if needed and commit the output */
    wlr_scene_output_commit(scene_output, NULL);

    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void on_output_destroy(struct wl_listener *listener, void *data)
{
    struct cwc_output *output = wl_container_of(listener, output, destroy_l);
    output->state->old_output = output;
    cwc_output_state_save(output);
    cwc_object_emit_signal_simple("screen::destroy", g_config_get_lua_State(),
                                  output);

    cwc_log(CWC_INFO, "destroying output (%s): %p %p", output->wlr_output->name,
            output, output->wlr_output);

    wl_list_remove(&output->destroy_l.link);
    wl_list_remove(&output->frame_l.link);
    wl_list_remove(&output->request_state_l.link);

    wl_list_remove(&output->config_commit_l.link);

    wl_list_remove(&output->link);

    luaC_object_unregister(g_config_get_lua_State(), output);

    free(output);
}

static void update_output_manager_config(struct cwc_output *output)
{
    // Probably won't work in multihead setup
    struct wlr_output_configuration_v1 *cfg =
        wlr_output_configuration_v1_create();
    wlr_output_configuration_head_v1_create(cfg, output->wlr_output);
    wlr_output_manager_v1_set_configuration(server.output_manager, cfg);
}

static void on_request_state(struct wl_listener *listener, void *data)
{
    struct cwc_output *output =
        wl_container_of(listener, output, request_state_l);
    struct wlr_output_event_request_state *event = data;

    wlr_output_commit_state(output->wlr_output, event->state);
    arrange_layers(output);
}

static void on_config_commit(struct wl_listener *listener, void *data)
{
    struct cwc_output *output =
        wl_container_of(listener, output, config_commit_l);
    struct cwc_config *old_config = data;

    if (old_config->useless_gaps == g_config.useless_gaps)
        return;

    cwc_output_tiling_layout_update_all_general_workspace(output);
}

static void on_new_output(struct wl_listener *listener, void *data)
{
    struct wlr_output *wlr_output = data;

    // TODO: multihead setup
    if (wl_list_length(&server.outputs) || wlr_output->non_desktop)
        return;

    /* Configures the output created by the backend to use our allocator
     * and our renderer. Must be done once, before commiting the output */
    wlr_output_init_render(wlr_output, server.allocator, server.renderer);

    /* The output may be disabled, switch it on. */
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    /* enable adaptive sync by default if supported */
    if (wlr_output->adaptive_sync_supported)
        wlr_output_state_set_adaptive_sync_enabled(&state, true);

    /* Some backends don't have modes. DRM+KMS does, and we need to set a mode
     * before we can use the output. The mode is a tuple of (width, height,
     * refresh rate), and each monitor supports only a specific set of modes. We
     * just pick the monitor's preferred mode, a more sophisticated compositor
     * would let the user configure it. */
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != NULL)
        wlr_output_state_set_mode(&state, mode);

    /* Atomically applies the new output state. */
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    struct cwc_output *output = calloc(1, sizeof(*output));
    output->type              = DATA_TYPE_OUTPUT;
    output->wlr_output        = wlr_output;
    output->wlr_output->data  = output;
    server.focused_output     = output;

    if (!cwc_output_state_try_restore(output))
        output->state = cwc_output_state_create();
    else
        output->restored = true;

    output->frame_l.notify         = on_output_frame;
    output->request_state_l.notify = on_request_state;
    output->destroy_l.notify       = on_output_destroy;
    wl_signal_add(&wlr_output->events.frame, &output->frame_l);
    wl_signal_add(&wlr_output->events.request_state, &output->request_state_l);
    wl_signal_add(&wlr_output->events.destroy, &output->destroy_l);

    output->config_commit_l.notify = on_config_commit;
    wl_signal_add(&g_config.events.commit, &output->config_commit_l);

    wl_list_insert(&server.outputs, &output->link);

    /* Adds this to the output layout. The add_auto function arranges outputs
     * from left-to-right in the order they appear. A more sophisticated
     * compositor would let the user configure the arrangement of outputs in the
     * layout.
     *
     * The output layout utility automatically adds a wl_output global to the
     * display, which Wayland clients can see to find out information about the
     * output (such as DPI, scale factor, manufacturer, etc).
     */
    struct wlr_output_layout_output *layout_output =
        wlr_output_layout_add_auto(server.output_layout, wlr_output);
    struct wlr_scene_output *scene_output =
        wlr_scene_output_create(server.scene, wlr_output);
    wlr_scene_output_layout_add_output(server.scene_layout, layout_output,
                                       scene_output);

    cwc_log(CWC_INFO, "created output (%s): %p %p", wlr_output->name, output,
            output->wlr_output);

    update_output_manager_config(output);
    arrange_layers(output);

    luaC_object_screen_register(g_config_get_lua_State(), output);
    cwc_object_emit_signal_simple("screen::new", g_config_get_lua_State(),
                                  output);
}

static void output_manager_apply(struct wlr_output_configuration_v1 *config,
                                 bool test)
{
    struct wlr_output_configuration_head_v1 *config_head;
    int ok = 1;

    cwc_log(CWC_DEBUG, "%sing new output config", test ? "test" : "apply");

    wl_list_for_each(config_head, &config->heads, link)
    {
        struct wlr_output *wlr_output = config_head->state.output;
        struct cwc_output *output     = wlr_output->data;
        struct wlr_output_state state;

        wlr_output_state_init(&state);
        wlr_output_state_set_enabled(&state, config_head->state.enabled);
        if (!config_head->state.enabled)
            goto apply_or_test;

        if (config_head->state.mode)
            wlr_output_state_set_mode(&state, config_head->state.mode);
        else
            wlr_output_state_set_custom_mode(
                &state, config_head->state.custom_mode.width,
                config_head->state.custom_mode.height,
                config_head->state.custom_mode.refresh);

        wlr_output_state_set_transform(&state, config_head->state.transform);
        wlr_output_state_set_scale(&state, config_head->state.scale);
        wlr_output_state_set_adaptive_sync_enabled(
            &state, config_head->state.adaptive_sync_enabled);

    apply_or_test:
        ok &= test ? wlr_output_test_state(wlr_output, &state)
                   : wlr_output_commit_state(wlr_output, &state);

        /* Don't move monitors if position wouldn't change, this to avoid
         * wlroots marking the output as manually configured.
         * wlr_output_layout_add does not like disabled outputs */
        if (!test)
            wlr_output_layout_add(server.output_layout, wlr_output,
                                  config_head->state.x, config_head->state.y);

        wlr_output_state_finish(&state);

        update_output_manager_config(output);
        arrange_layers(output);
        cwc_output_tiling_layout_update(output, 0);
    }

    if (ok)
        wlr_output_configuration_v1_send_succeeded(config);
    else
        wlr_output_configuration_v1_send_failed(config);
    wlr_output_configuration_v1_destroy(config);
}

static void on_output_manager_test(struct wl_listener *listener, void *data)
{
    struct wlr_output_configuration_v1 *config = data;

    output_manager_apply(config, true);
}

static void on_output_manager_apply(struct wl_listener *listener, void *data)
{
    struct wlr_output_configuration_v1 *config = data;

    output_manager_apply(config, false);
}

static void on_opm_set_mode(struct wl_listener *listener, void *data)
{
    struct wlr_output_power_v1_set_mode_event *event = data;

    struct wlr_output_state state;
    wlr_output_state_init(&state);

    wlr_output_state_set_enabled(&state, event->mode);
    wlr_output_commit_state(event->output, &state);

    wlr_output_state_finish(&state);
}

void setup_output(struct cwc_server *s)
{
    // wlr output layout
    s->output_layout       = wlr_output_layout_create(s->wl_display);
    s->new_output_l.notify = on_new_output;
    wl_signal_add(&s->backend->events.new_output, &s->new_output_l);

    // output manager
    s->output_manager = wlr_output_manager_v1_create(s->wl_display);
    s->output_manager_test_l.notify  = on_output_manager_test;
    s->output_manager_apply_l.notify = on_output_manager_apply;
    wl_signal_add(&s->output_manager->events.test, &s->output_manager_test_l);
    wl_signal_add(&s->output_manager->events.apply, &s->output_manager_apply_l);

    // output power manager
    s->output_power_manager = wlr_output_power_manager_v1_create(s->wl_display);
    s->opm_set_mode_l.notify = on_opm_set_mode;
    wl_signal_add(&s->output_power_manager->events.set_mode,
                  &s->opm_set_mode_l);
}

void cwc_output_update_visible(struct cwc_output *output)
{
    struct cwc_container *container;
    wl_list_for_each(container, &output->state->containers,
                     link_output_container)
    {
        if (cwc_container_is_visible(container)) {
            cwc_container_set_enabled(container, true);
        } else {
            cwc_container_set_enabled(container, false);
        }
    }

    cwc_output_focus_newest_focus_visible_toplevel(output);
}

struct cwc_output *cwc_output_get_focused()
{
    return server.focused_output;
}

struct cwc_toplevel *cwc_output_get_newest_toplevel(struct cwc_output *output,
                                                    bool visible)
{
    struct cwc_toplevel *toplevel;
    wl_list_for_each(toplevel, &output->state->toplevels, link_output_toplevels)
    {

        if (cwc_toplevel_is_unmanaged(toplevel))
            continue;

        if (visible && !cwc_toplevel_is_visible(toplevel))
            continue;

        return toplevel;
    }

    return NULL;
}

struct cwc_toplevel *
cwc_output_get_newest_focus_toplevel(struct cwc_output *output, bool visible)
{
    struct cwc_container *container;
    wl_list_for_each(container, &output->state->focus_stack, link_output_fstack)
    {
        struct cwc_toplevel *toplevel =
            cwc_container_get_front_toplevel(container);
        if (cwc_toplevel_is_unmanaged(toplevel))
            continue;

        if (visible && !cwc_toplevel_is_visible(toplevel))
            continue;

        return toplevel;
    }

    return NULL;
}

void cwc_output_focus_newest_focus_visible_toplevel(struct cwc_output *output)
{
    struct cwc_toplevel *toplevel =
        cwc_output_get_newest_focus_toplevel(output, true);

    if (toplevel) {
        cwc_toplevel_focus(toplevel, false);
        return;
    }

    wlr_seat_pointer_clear_focus(server.seat->wlr_seat);
    wlr_seat_keyboard_clear_focus(server.seat->wlr_seat);
}

bool cwc_output_is_exist(struct cwc_output *output)
{
    struct cwc_output *_output;
    wl_list_for_each(_output, &server.outputs, link)
    {
        if (_output == output)
            return true;
    }

    return false;
}

//=========== MACRO ===============

struct cwc_output *
cwc_output_at(struct wlr_output_layout *ol, double x, double y)
{
    struct wlr_output *o = wlr_output_layout_output_at(ol, x, y);
    return o ? o->data : NULL;
}

struct cwc_toplevel **
cwc_output_get_visible_toplevels(struct cwc_output *output)
{
    int maxlen                 = wl_list_length(&output->state->toplevels);
    struct cwc_toplevel **list = calloc(maxlen + 1, sizeof(void *));

    int tail_pointer = 0;
    struct cwc_toplevel *toplevel;
    wl_list_for_each(toplevel, &output->state->toplevels, link_output_toplevels)
    {
        if (cwc_toplevel_is_visible(toplevel)) {
            list[tail_pointer] = toplevel;
            tail_pointer += 1;
        }
    }

    return list;
}

struct cwc_container **
cwc_output_get_visible_containers(struct cwc_output *output)
{
    int maxlen                  = wl_list_length(&output->state->containers);
    struct cwc_container **list = calloc(maxlen + 1, sizeof(void *));

    int tail_pointer = 0;
    struct cwc_container *container;
    wl_list_for_each(container, &output->state->containers,
                     link_output_container)
    {
        if (cwc_container_is_visible(container)) {
            list[tail_pointer] = container;
            tail_pointer += 1;
        }
    }

    return list;
}

//================== TAGS OPERATION ===================

/** set to specified view reseting all tagging bits or in short switch to
 * workspace x
 */
void cwc_output_set_view_only(struct cwc_output *output, int view)
{
    output->state->active_tag       = 1 << (view - 1);
    output->state->active_workspace = view;

    cwc_output_tiling_layout_update(output, 0);
    cwc_output_update_visible(output);
}

static void restore_floating_box_for_all(struct cwc_output *output)
{
    struct cwc_container *container;
    wl_list_for_each(container, &output->state->containers,
                     link_output_container)
    {
        if (cwc_container_is_floating(container)
            && cwc_container_is_visible(container)
            && cwc_container_is_configure_allowed(container))
            cwc_container_restore_floating_box(container);
    }
}

void cwc_output_set_layout_mode(struct cwc_output *output,
                                enum cwc_layout_mode mode)
{

    if (mode < 0 || mode >= CWC_LAYOUT_LENGTH)
        return;

    enum cwc_layout_mode *current_mode =
        &cwc_output_get_current_view_info(output)->layout_mode;
    *current_mode = mode;

    switch (mode) {
    case CWC_LAYOUT_BSP:
        insert_tiled_toplevel_to_bsp_tree(output,
                                          output->state->active_workspace);
        break;
    case CWC_LAYOUT_FLOATING:
        restore_floating_box_for_all(output);
        break;
    default:
        break;
    }

    cwc_output_tiling_layout_update(output, 0);
}

void cwc_output_set_strategy_idx(struct cwc_output *output, int idx)
{
    struct cwc_view_info *info = cwc_output_get_current_view_info(output);
    enum cwc_layout_mode current_mode = info->layout_mode;

    switch (current_mode) {
    case CWC_LAYOUT_BSP:
        break;
    case CWC_LAYOUT_MASTER:
        if (idx > 0)
            while (idx--)
                info->master_state.current_layout =
                    info->master_state.current_layout->next;
        else if (idx < 0)
            while (idx++)
                info->master_state.current_layout =
                    info->master_state.current_layout->prev;
        master_arrange_update(output);
        break;
    default:
        return;
    }
}

void cwc_output_set_useless_gaps(struct cwc_output *output,
                                 int workspace,
                                 int gaps_width)
{
    if (!workspace)
        workspace = output->state->active_workspace;

    workspace  = CLAMP(workspace, 1, MAX_WORKSPACE);
    gaps_width = MAX(0, gaps_width);

    output->state->view_info[workspace].useless_gaps = gaps_width;
    cwc_output_tiling_layout_update(output, workspace);
}

void cwc_output_set_mwfact(struct cwc_output *output,
                           int workspace,
                           double factor)
{
    if (!workspace)
        workspace = output->state->active_workspace;

    workspace = CLAMP(workspace, 1, MAX_WORKSPACE);
    factor    = CLAMP(factor, 0.1, 0.9);

    output->state->view_info[workspace].master_state.mwfact = factor;
    cwc_output_tiling_layout_update(output, workspace);
}
