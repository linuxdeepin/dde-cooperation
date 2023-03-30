project(
  'dde-cooperation',
  'cpp',
  version: '0.0.1',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
  ],
)

cpp_compiler = meson.get_compiler('cpp')
if cpp_compiler.get_argument_syntax() == 'clang'
  compiler_args = cpp_compiler.get_supported_arguments([
  ])
else
  compiler_args = cpp_compiler.get_supported_arguments([
    '-Wno-c++20-extensions',
  ])
endif
compiler_args += cpp_compiler.get_supported_arguments([
  '-Wno-vla',
])
add_project_arguments(compiler_args, language: 'cpp')

prefixdir = get_option('prefix')
datadir = prefixdir / get_option('datadir')
libexecdir = prefixdir / get_option('libexecdir')

cxx = meson.get_compiler('cpp')
stdcxxfs = cxx.find_library('stdc++fs', required: false)

thread = dependency('threads')
uuid = dependency('uuid', required: true)
fmt = dependency('fmt', required: true)
tl_expected = dependency('tl-expected', method: 'cmake', modules: ['tl::expected'], required: true)
libevdev = dependency('libevdev', required: true)
fuse3 = dependency('fuse3', required: true)
xcb = dependency('xcb', required: true)
xcb_randr = dependency('xcb-randr', required: true)
xcb_xinput = dependency('xcb-xinput', required: true)
xcb_xfixes = dependency('xcb-xfixes', required: true)
qrcodegencpp = dependency('qrcodegencpp', required: true)

qt5 = import('qt5')
qt5dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets', 'Network', 'DBus', 'X11Extras', 'Multimedia', 'QuickWidgets'])
dtk_widget = dependency('dtkwidget', required: true)
dtk_core = dependency('dtkcore', required: true)

libavcodec = dependency('libavcodec', required: true)
libavformat = dependency('libavformat', required: true)
libavutil = dependency('libavutil', required: true)
libswscale = dependency('libswscale', required: true)

protobuf = dependency('protobuf', required: true)
protoc_exec = find_program('protoc')
protoc = generator(protoc_exec,
  output: ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments: ['--proto_path=@CURRENT_SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '--experimental_allow_proto3_optional', '@INPUT@']
)

systemd = dependency('systemd')
systemd_user_unit_dir = systemd.get_pkgconfig_variable('systemduserunitdir')

executable_install_dir = join_paths(libexecdir, 'dde-cooperation')

subdir('src')

install_data('misc/dbus/services/org.deepin.dde.Cooperation1.service',
             install_dir: join_paths(datadir, 'dbus-1', 'services'))

data = configuration_data()
data.set('executable_install_dir', executable_install_dir )

configure_file(
  input: 'misc/systemd-service/dde-cooperation.service.in',
  output: '@BASENAME@',
  configuration: data,
  install_dir: systemd_user_unit_dir,
)

translationSourceFiles = files([
   'translations/dde-cooperation_en.ts',
   'translations/dde-cooperation_zh_CN.ts',
   'translations/dde-cooperation_zh_HK.ts',
   'translations/dde-cooperation_zh_TW.ts',
])
translations = qt5.compile_translations(
  ts_files: translationSourceFiles,
  build_by_default: true,
  install: true,
  install_dir: datadir / 'dde-cooperation' / 'translations',
)

install_data('misc/org.deepin.dde.cooperation.desktop',
             install_dir: join_paths(datadir, 'applications'))

install_data('misc/org.deepin.dde.cooperation.svg',
             install_dir: join_paths(datadir, 'icons', 'hicolor', 'scalable', 'apps'))

install_data('misc/config/compressor-singlefile.conf',
             install_dir: '/usr/share/applications/context-menus')

install_data('misc/sendfile/sendFiles.sh',
             install_dir: executable_install_dir)

install_data('configs/org.deepin.cooperation.json',
             install_dir: '/usr/share/dsg/configs/org.deepin.cooperation')
