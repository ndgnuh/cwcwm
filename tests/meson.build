executable(
  'hashc',
  ['hash.c', '../src/util-map.c'],
  dependencies: [xxhash, wlr, wayland_server],
  include_directories : cwc_inc,
)

executable(
  'hashcpp',
  ['hash.cpp'],
)

shared_module(
  'csignal_test', 'capi/signal.c',
  protocols_server_header['xdg-shell'],
  include_directories : cwc_inc,
  dependencies: [wlr, lua],
  name_prefix: '',
)
