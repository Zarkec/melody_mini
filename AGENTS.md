# Repository Guidelines

## 项目结构与模块组织

本仓库是一个使用 CMake 构建的 Qt 6 C++ 桌面应用。应用源码位于 `src/`：`src/main.cpp` 负责启动程序，`src/ui/` 存放窗口和控件 UI 逻辑，`src/core/` 存放播放、API 和播放列表相关逻辑。静态资源位于 `resources/`，包括 `resources.qrc`、`resources/icons/` 下的图标、`logo.png` 以及 Windows 资源文件 `app.rc`。构建产物应保留在 `build/` 中，不要提交到版本库。

## 构建、测试与开发命令

- `cmake -S . -B build`：配置项目并查找 Qt 6.5+ 组件。
- `cmake --build build`：编译 `melody` 可执行文件。
- `cmake --install build --prefix dist`：按需生成安装或部署目录。

请在仓库根目录运行上述命令。如果 CMake 找不到 Qt，请将 `CMAKE_PREFIX_PATH` 指向 Qt 安装路径，例如 `-DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64`。

## 代码风格与命名约定

项目使用 C++17，并遵循现有 Qt 代码习惯。类名使用 `PascalCase`，例如 `Widget`、`PlaylistManager`；方法和变量使用 `camelCase`；辅助控件中的私有成员通常使用 `m_` 前缀。头文件保持 `#ifndef`/`#define` 宏保护，并显式包含所需 Qt 头文件。信号和槽命名应清晰，例如 `onSearchFinished`、`playNextSong`。保持现有四空格缩进，并遵循 Qt 父子对象生命周期管理方式。

## 测试指南

当前尚未配置自动化测试目标或测试框架。涉及核心逻辑的改动，建议后续在 `tests/` 目录中添加聚焦测试，并通过 CTest 接入。在测试框架完善前，请至少运行 `cmake --build build` 完成编译验证，并手动检查搜索、播放控制、音量和设备切换、分页、托盘操作以及悬浮灵动岛 UI。

## 提交与 Pull Request 规范

近期提交历史使用类似 Conventional Commits 的前缀，例如 `feat:` 和 `refactor:`。提交标题应简短、明确，例如 `feat: add playlist shuffle mode` 或 `fix: handle empty API responses`。Pull Request 应说明用户可感知的变化，列出手动验证步骤，关联相关 issue；涉及 UI 变化时，请附截图或简短录屏。

## 安全与配置提示

不要提交生成的二进制文件、本地 Qt Creator 设置、API 密钥或临时媒体文件。网络和 API 相关行为应尽量集中在 `src/core/apimanager.*` 中，避免在源码或资源文件中硬编码凭据。
