#ifndef _CWC_IDLE_H
#define _CWC_IDLE_H

#include <wlr/types/wlr_idle_inhibit_v1.h>
#include <wlr/types/wlr_idle_notify_v1.h>

struct cwc_idle {
    struct cwc_server *server;
    struct wlr_idle_notifier_v1 *idle_notifier;
    struct wlr_idle_inhibit_manager_v1 *inhibit_manager;
    struct wl_list inhibitors; // struct cwc_idle_inhibitor.link

    struct wl_listener new_inhibitor_l;
};

struct cwc_idle_inhibitor {
    struct wl_list link; // cwc_idle.inhibitors
    struct cwc_idle *cwc_idle;
    struct wlr_idle_inhibitor_v1 *wlr_inhibitor;
    struct wl_listener destroy_inhibitor_l;
};

#endif // !_CWC_IDLE_H
