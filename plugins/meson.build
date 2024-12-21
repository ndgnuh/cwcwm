plugdir = cwc_datadir / 'plugins'

shared_module(
  'cwcle', 'cwcle.c', 
  protocols_server_header['xdg-shell'],
  include_directories : cwc_inc,
  dependencies: [wlr, cairo, lua],
  name_prefix: '',
  install: true,
  install_dir: plugdir
)

shared_module(
  'flayout', 'flayout.c', 
  protocols_server_header['xdg-shell'],
  include_directories : cwc_inc,
  dependencies: [wlr],
  name_prefix: '',
  install: true,
  install_dir: plugdir 
)
