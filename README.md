# 全能网页视频下载器 (Universal Video Downloader)

![Platform](https://img.shields.io/badge/Platform-macOS%20%7C%20Windows-blue)
![Qt](https://img.shields.io/badge/Framework-Qt%206-green)
![C++](https://img.shields.io/badge/Language-C%2B%2B%2020-orange)

基于 **Qt C++** 开发的跨平台视频下载工具，深度集成 `yt-dlp`、`FFmpeg` 和 `aria2c`，提供简洁的图形化界面，支持绕过主流视频网站反爬虫机制、磁力/种子下载和断点续传。

---

## ✨ 核心特性

| 功能 | 说明 |
|------|------|
| 🧲 **磁力/BT 下载** | 集成 aria2c，支持 magnet 链接和 .torrent 文件下载 |
| 🔄 **断点续传** | yt-dlp 和 aria2c 均支持断网后自动重试（最多 3 次）并继续未完成的下载 |
| 🔍 **自动解析网页** | 自动抓取网页 HTML，从 JS/JSON 中提取隐藏的 m3u8/mp4 视频链接 |
| 🖥️ **跨平台支持** | 原生支持 macOS (Apple Silicon) 和 Windows 10/11 |
| 🍪 **Cookie 注入** | 从浏览器提取登录凭证，解锁 B站 1080P/4K 大会员画质 |
| 📂 **合集批量下载** | 支持 B站分P、YouTube 播放列表一键下载，自动按序号命名 |
| 🔊 **智能音视频合并** | 自动调用 FFmpeg 合并分离的音视频流，输出完整 `.mp4` |
| ⬇️ **自动下载组件** | Windows 端首次启动自动下载 yt-dlp、ffmpeg 和 aria2c |
| ⏹ **随时停止** | 支持在下载过程中随时终止任务 |

---

## 📦 项目结构

```
├── main.cpp              # 程序入口
├── mainwindow.h/cpp      # 主窗口 UI 与业务逻辑
├── url_extractor.h/cpp   # 网页解析模块（HTML 抓取 + 视频 URL 提取）
├── platformutils.h/cpp   # 平台抽象层（路径查找、平台检测）
├── toolmanager.h/cpp     # 工具下载管理器（自动下载/解压）
├── CMakeLists.txt        # 构建配置
└── .github/workflows/    # CI/CD 自动构建
```

---

## 🚀 快速开始

### macOS

#### 1. 安装依赖

```bash
# 安装 Homebrew（如未安装）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装必要工具
brew install yt-dlp ffmpeg aria2 cmake qt@6
```

#### 2. 编译运行

```bash
cd M3u8Downloader

# 配置并编译
cmake -S . -B build
cmake --build build

# 运行
open build/UniversalVideoDownloader.app
```

---

### Windows

#### 方式一：自动下载（推荐）

1. 从 GitHub Actions 下载最新构建产物并解压
2. 双击运行 `UniversalVideoDownloader.exe`
3. 程序会自动检测并提示下载缺失的 yt-dlp、ffmpeg 和 aria2c

#### 方式二：手动配置

1. 下载 [yt-dlp.exe](https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe)
2. 下载 [ffmpeg.exe](https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip) 并解压
3. 下载 [aria2c.exe](https://github.com/aria2/aria2/releases) 并解压
4. 将 `yt-dlp.exe`、`ffmpeg.exe` 和 `aria2c.exe` 放入程序所在目录

---

## 📖 使用指南

### 基本下载

1. 在输入框粘贴视频链接（支持 B站、YouTube 等）
2. 点击 **🔍 解析视频** 等待解析完成
3. 选择画质（推荐 **自动最佳画质**）
4. 点击 **⬇️ 开始下载**

### 磁力/BT 下载

1. 在输入框粘贴 `magnet:?xt=...` 链接，或输入 `.torrent` 文件的绝对路径
2. 程序自动识别为 BT 任务，跳过解析步骤
3. 点击 **⬇️ 开始下载**，aria2c 引擎将启动 DHT 网络下载
4. 下载过程中可点击 **⏹ 停止** 随时终止

> 💡 aria2c 自动启用 16 线程分片下载和 DHT/PEX 加速

### 断点续传

- **yt-dlp 引擎**：视频下载中断后，重新点击下载会自动从断点继续
- **aria2c 引擎**：BT/磁力下载天然支持续传，重启后继续未完成的任务
- **自动重试**：网络中断时程序会自动等待 3 秒后重试，最多重试 3 次

### 自动解析网页（提取隐藏链接）

部分网站会将视频 URL（如 m3u8）隐藏在 HTML/JavaScript 中，地址栏显示的是网页地址而非视频直链。

1. 确保 **自动解析** 复选框已勾选（默认开启）
2. 粘贴网页地址，点击解析
3. 程序会自动抓取网页 HTML，从中匹配并提取 m3u8/mpd/mp4 等媒体流地址
4. 提取成功后自动交给 yt-dlp 获取详细信息

> 💡 如果自动解析未找到链接，会自动回退到 yt-dlp 直接解析

### Cookie 绕过（解锁高清画质）

1. 在 **使用 Cookie** 下拉菜单选择你登录的浏览器
2. 确保浏览器已登录目标网站账号
3. 点击解析，程序会自动获取最高画质

> 💡 推荐使用 Firefox，兼容性最佳

### 合集批量下载

1. 勾选 **合集模式**
2. 粘贴 B站分P链接或 YouTube 播放列表链接
3. 点击解析，程序会自动锁定最佳画质
4. 点击下载，文件会按 `01_标题.mp4` 格式自动命名

---

## 🛠️ 技术架构

```
┌──────────────────────────────────────────────────────────┐
│                   MainWindow (Qt GUI)                     │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐ │
│  │  URL 输入     │   │  画质选择     │   │  进度/日志    │ │
│  └──────┬───────┘   └──────┬───────┘   └──────────────┘ │
│         │                   │                             │
│  ┌──────┴───────┐   ┌──────┴───────┐                     │
│  │ UrlExtractor │   │   QProcess   │                     │
│  │ HTML 抓取    │   │  yt-dlp 调用  │                     │
│  │ URL 提取     │   │  aria2c 调用  │                     │
│  └──────┬───────┘   │  FFmpeg 合并  │                     │
│         │           └──────┬───────┘                     │
│  ┌──────┴──────────────────┴────────────────────────────┐│
│  │              PlatformUtils                            ││
│  │  • 平台检测    • yt-dlp/ffmpeg/aria2c 路径查找         ││
│  └──────────────────────┬───────────────────────────────┘│
│                         │                                 │
│  ┌──────────────────────┴───────────────────────────────┐│
│  │              ToolManager (Windows)                    ││
│  │  • 自动下载    • 解压管理    • 状态通知                ││
│  └──────────────────────────────────────────────────────┘│
└──────────────────────────────────────────────────────────┘
```

### 核心技术栈

- **语言**: C++20
- **GUI 框架**: Qt 6
- **构建系统**: CMake
- **网络请求**: QNetworkAccessManager（网页抓取）
- **外部引擎**: yt-dlp (视频解析/下载) + FFmpeg (音视频合并) + aria2c (BT/磁力下载)
- **CI/CD**: GitHub Actions

### 关键设计

- **UrlExtractor**: 网页解析模块，通过 HTTP GET 拉取 HTML 源码，利用正则表达式匹配嵌套在 JS/JSON 中的媒体流地址（m3u8/mpd/mp4），支持反转义处理（`\/` → `/`、`\u002F` → `/` 等）
- **PlatformUtils**: 平台抽象层，封装跨平台路径查找逻辑（yt-dlp / ffmpeg / aria2c）
- **ToolManager**: 工具管理器，负责自动下载、解压和状态管理，链式下载队列：yt-dlp → ffmpeg → aria2c
- **双引擎下载**: 普通视频链接走 yt-dlp（支持 Cookie/画质选择），磁力/种子链接走 aria2c（16 线程 + DHT 加速）
- **断点续传**: yt-dlp `--continue` + aria2c `--continue=true`，网络中断自动重试最多 3 次
- **QProcess**: 异步调用外部 CLI 引擎，避免阻塞 UI

---

## 🔧 常见问题

### 下载后没有声音？

- **macOS**: 确认已执行 `brew install ffmpeg`
- **Windows**: 确认 `ffmpeg.exe` 在程序目录下（或使用自动下载功能）

### B站解析失败？

- 确保浏览器已登录 B站账号
- 在 Cookie 下拉菜单选择正确的浏览器
- 尝试关闭浏览器后重试

### 自动解析未找到链接？

- 部分网站的视频链接通过 JavaScript 动态加载，静态 HTML 中不包含
- 此时程序会自动回退到 yt-dlp 直接解析
- 可以取消勾选 **自动解析** 跳过此步骤

### 磁力/BT 下载慢？

- 初次连接 DHT 网络需要一定时间，请耐心等待
- 确认 aria2c 已安装（macOS: `brew install aria2`）
- 冷门资源可能需要较长时间做种

### 下载中断了怎么办？

- 直接重新点击下载，程序会自动从断点继续
- 如果多次中断，程序会自动重试最多 3 次
- BT/磁力任务天然支持续传，无需额外操作

---

## 📄 许可证

本项目仅供学习和个人使用，请勿用于商业用途。

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

---

<div align="center">
  <sub>Built with ❤️ by ChoBits</sub>
</div>
