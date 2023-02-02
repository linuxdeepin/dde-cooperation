dde_cooperation_sources = files('''
  main.cc
  Manager.cc
  ManagerDBusAdaptor.cc
  ConfirmDialog.cc
  Machine/Machine.cc
  Machine/MachineDBusAdaptor.cc
  Machine/PCMachine.cc
  Machine/AndroidMachine.cc
  Machine/AndroidMachineDBusAdaptor.cc
  Wrappers/InputEmitterWrapper.cc
  Wrappers/InputGrabberWrapper.cc
  Wrappers/InputGrabbersManager.cc
  Fuse/FuseClient.cc
  Fuse/FuseServer.cc
  ReconnectDialog.cc
  SendTransfer.cc
  ReceiveTransfer.cc
  DisplayBase.h
  DisplayBase.cc
  ClipboardBase.h
  ClipboardBase.cc
  X11/X11.cc
  X11/Display.h
  X11/Display.cc
  X11/Clipboard.h
  X11/Clipboard.cc
  Android/QrCodeProxy.cc
  Android/DeviceProxy.cc
  Android/VideoFrameProvider.cc
  Android/AndroidMainWindow.cc
'''.split())

dde_cooperation_moc_headers = files('''
  Manager.h
  ManagerDBusAdaptor.h
  ConfirmDialog.h
  Machine/Machine.h
  Machine/PCMachine.h
  Machine/AndroidMachine.h
  Machine/MachineDBusAdaptor.h
  Machine/AndroidMachineDBusAdaptor.h
  Wrappers/InputEmitterWrapper.h
  Wrappers/InputGrabberWrapper.h
  Wrappers/InputGrabbersManager.h
  Fuse/FuseClient.h
  Fuse/FuseServer.h
  ReconnectDialog.h
  SendTransfer.h
  ReceiveTransfer.h
  X11/X11.h
  Android/QrCodeProxy.h
  Android/DeviceProxy.h
  Android/VideoFrameProvider.h
  Android/AndroidMainWindow.h
'''.split())

dde_cooperation_sources += qt5.preprocess(
  qresources: [
    'Android/resources.qrc',
  ],
  moc_headers: dde_cooperation_moc_headers,
)

dde_cooperation_sources += common_sources + [protocol] + [ipc_protocol]

conf_data = configuration_data()
conf_data.set('executable_install_dir', executable_install_dir)
configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: conf_data
)

executable('dde-cooperation',
  dde_cooperation_sources,
  include_directories: [
    includes,
  ],
  dependencies: [
    stdcxxfs,
    thread,
    uuid,
    fmt,
    libevdev,
    tl_expected,
    fuse3,
    xcb,
    xcb_randr,
    xcb_xinput,
    xcb_xfixes,
    protobuf,
    qt5dep,
    dtk_core,
    dtk_widget,
    qrcodegencpp,
    scrcpy_core_dep,
  ],
  install: true,
  install_dir: executable_install_dir
)
