# Windows 构建快速指南

## 一键构建

### 64位版本 (推荐用于 Windows 10/11)
```batch
wix_build.bat 1.1.23 x64
```

### 32位版本 (用于 Windows 7)
```batch
wix_build.bat 1.1.23 x86
```

## 前置要求

1. Visual Studio 2019/2022
2. Qt 5.15.2 (MSVC版本)
3. OpenSSL 3.x
4. WiX Toolset 4+ (可选)
   ```powershell
   winget install WiX.Toolset
   ```

## 输出

MSI安装包位于:
```
build/_CPack_Packages/win-{架构}/WIX/
├── deepin-cooperation-{版本}-win-{架构}.msi
└── deepin-datatransfer-{版本}-win-{架构}.msi
```

## 详细文档

完整文档请参考: [docs/BUILD_WINDOWS.md](../../../docs/BUILD_WINDOWS.md)

## CI/CD集成

```yaml
- name: Build
  run: cmake --build build --config Release --target package
```

## 故障排查

- **WiX未找到**: `winget install WiX.Toolset`
- **DLL缺失**: 检查OpenSSL路径
- **32位构建失败**: 确保使用32位Qt和OpenSSL

## 架构对比

| 架构 | 目标系统 | Qt版本 | OpenSSL |
|------|---------|--------|---------|
| x64  | Win10/11 | msvc2019_64 | C:\Program Files\OpenSSL |
| x86  | Win7+    | msvc2019_86 | C:\Program Files (x86)\OpenSSL |
