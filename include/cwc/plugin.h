#ifndef _CWC_PLUGIN_H
#define _CWC_PLUGIN_H

#include <stdbool.h>
#include <wayland-util.h>

struct cwc_server;

typedef int (*initcall_t)(void);
typedef void (*exitcall_t)(void);

struct cwc_plugin {
    struct wl_list link; // cwc_server.plugins

    // metadata
    char *name;
    char *description;
    char *version;
    char *author;
    char *license;
    char *filename;

    void *handle;
    initcall_t init_fn;
};

#define __PASTE(a, b) a##b

#define __stringify_1(s...) #s

#define __stringify(s...) __stringify_1(s)

#define __section(name) __attribute__((__section__(name)))

#define __aligned(x) __attribute__((__aligned__(x)))

#define __used __attribute__((__used__))

#define __PLUGIN_TAG_SYMBOL(tag) _pluginfo_##tag

#define PLUGIN_TAG_SYMBOL(tag) __stringify(__PLUGIN_TAG_SYMBOL(tag))

#define PLUGIN_INFO(tag, value)                               \
    const char __PLUGIN_TAG_SYMBOL(tag)[] __used __aligned(1) \
        __section(".pluginfo") = #tag "=" value

/* A wrapper to mark function as plugin entry point */
#define plugin_init(fn)     \
    int __cwc_init_plugin() \
    {                       \
        return fn();        \
    }

#define plugin_init_global(fn)     \
    int __cwc_init_plugin_global() \
    {                              \
        return fn();               \
    }

/* invoked when plugin removed */
#define plugin_exit(fn)         \
    void __cwc_cleanup_plugin() \
    {                           \
        return fn();            \
    }

/* set plugin name (required) */
#define PLUGIN_NAME(_name) PLUGIN_INFO(name, _name)

/* Set plugin version (required) */
#define PLUGIN_VERSION(_version) PLUGIN_INFO(version, _version)

/* Set plugin license */
#define PLUGIN_LICENSE(_license) PLUGIN_INFO(license, _license)

/* Set plugin author */
#define PLUGIN_AUTHOR(_author) PLUGIN_INFO(author, _author)

/* Set plugin description */
#define PLUGIN_DESCRIPTION(_description) PLUGIN_INFO(description, _description)

struct cwc_plugin *__load_plugin(const char *pathname, int __mode);

/* load plugin */
struct cwc_plugin *load_plugin(const char *pathname);

/* load plugin as library (the symbol exposed globally to other plugin) */
struct cwc_plugin *load_plugin_library(const char *pathname);

/* start plugin */
void cwc_plugin_start(struct cwc_plugin *plugin);

/* run the cleanup function and destroy it */
void cwc_plugin_unload(struct cwc_plugin *plugin);

/* return true if found */
bool cwc_plugin_stop_by_name(const char *name);

bool cwc_plugin_is_exist(const char *name);

/* stop all plugin in the list */
void cwc_plugin_stop_plugins(struct wl_list *head);

#endif // !_CWC_PLUGIN_H
