# SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

input_emitter_sources = files('''
  main.cc
  InputEmitter.h
  InputEmitter.cc
'''.split())

input_emitter_moc_headers = files('''
'''.split())

input_emitter_sources += qt5.preprocess(
  qresources: [],
  moc_headers: input_emitter_moc_headers,
)

input_emitter_sources += [ipc_protocol]

input_emitter = executable('input-emitter',
  input_emitter_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    stdcxxfs,
    thread,
    fmt,
    libevdev,
    protobuf,
    qt5dep,
  ],
  install: true,
  install_mode: ['rwsr-xr-x'],
  install_dir: executable_install_dir
)
