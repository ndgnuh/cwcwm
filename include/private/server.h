struct cwc_server;

extern void setup_output(struct cwc_server *s);
extern void setup_xdg_shell(struct cwc_server *s);
extern void setup_decoration_manager(struct cwc_server *_s);

extern void setup_cwc_session_lock(struct cwc_server *s);
extern void setup_layer_shell(struct cwc_server *s);

extern void setup_keyboard(struct cwc_server *s);
extern void setup_pointer(struct cwc_server *s);

extern void cwc_idle_init(struct cwc_server *s);
extern void cwc_idle_fini(struct cwc_server *s);
extern void xwayland_init(struct cwc_server *s);
extern void xwayland_fini(struct cwc_server *s);
