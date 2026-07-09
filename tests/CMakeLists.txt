# 项目单元测试入口 (DOTEST 模式)
# 参考 deepin-system-monitor: 集中在根 tests/ 下, 子目录按 target 隔离。
add_subdirectory(logic)
add_subdirectory(coop)
add_subdirectory(coop_gui)
add_subdirectory(compat)
add_subdirectory(dt)
add_subdirectory(slotipc)
add_subdirectory(logging)
