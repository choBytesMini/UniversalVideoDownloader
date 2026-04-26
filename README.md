# Universal Video Downloader

[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows%20%7C%20Linux-lightgrey)](https://github.com/choBytesMini/UniversalVideoDownloader)
[![Qt](https://img.shields.io/badge/Qt-6.5%2B-green)](https://www.qt.io/)
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-MIT-yellow)](LICENSE)

基于 Qt6 C++20 的跨平台视频下载工具，集成 yt-dlp、FFmpeg、aria2c 三大引擎，提供图形界面与命令行双模式。

---

## 支持平台

| 平台 | 工具安装 | 自动下载组件 |
|------|----------|-------------|
| macOS (ARM / x86_64) | `brew install yt-dlp ffmpeg aria2` | 否 |
| Windows 10/11 | 手动下载 或 程序自动下载 | 是 |
| Linux | 系统包管理器 | 否 |

## 支持的链接类型

| 类型 | 引擎 | 示例 |
|------|------|------|
| 主流视频网站 | yt-dlp | B站、YouTube、Twitter、TikTok 等 |
| 磁力链接 | aria2c | `magnet:?xt=urn:btih:...` |
| 种子文件 | aria2c | `file:///path/to/video.torrent` |
| Info Hash | aria2c | 40 位十六进制字符串，自动转为磁力 |
| 网页中的媒体流 | 自动提取 → yt-dlp | 从 HTML/JSON/JS 中提取 m3u8、mpd、mp4 等 |

## 功能

- **双引擎下载** — 普通视频走 yt-dlp（支持 Cookie 注入、画质选择），磁力/种子走 aria2c（DHT + PEX 加速）
- **Cookie 注入** — 支持 Firefox / Chrome / Edge / Safari 浏览器 Cookie，解锁 B站大会员等登录限定画质
- **自动解析网页** — 从 HTML 源码中提取隐藏在 JSON/JS/`<video>` 标签中的 m3u8/mpd/mp4 媒体地址
- **合集批量下载** — B站分P、YouTube 播放列表一键下载，自动编号命名
- **断点续传** — yt-dlp 和 aria2c 均支持断网续传，自动重试最多 3 次
- **音视频合并** — 自动调用 FFmpeg 合并分离的音视频流为完整 MP4
- **系统外观自适应** — 自动跟随 macOS/Windows 深色/亮色模式
- **Windows 自动部署** — 首次启动自动下载 yt-dlp.exe、ffmpeg.exe、aria2c.exe
- **命令行模式** — `--cli` 参数进入终端模式，无 GUI 开销，适合远程服务器和脚本

---

## 安装

### macOS

```bash
# 安装依赖
brew install yt-dlp ffmpeg aria2 cmake qt@6

# 编译
cd M3u8Downloader
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 运行 GUI
open build/UniversalVideoDownloader.app

# 运行 CLI
./build/UniversalVideoDownloader.app/Contents/MacOS/UniversalVideoDownloader -c <URL>
```

### Windows

**自动安装**：从 [Releases](../../releases) 下载 `UniversalVideoDownloader.exe`，双击运行，程序会自动检测并下载缺失组件。

**手动编译**：

```powershell
# 安装 Qt 6.5+ 和 CMake，确保 qmake 在 PATH 中
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

**手动安装工具**：将 `yt-dlp.exe`、`ffmpeg.exe`、`aria2c.exe` 放到 `UniversalVideoDownloader.exe` 同级目录。

---

## 使用

### GUI 模式

1. 粘贴视频链接或磁力链接
2. 选择 Cookie 来源（如需要登录）
3. 点击 **解析视频**，等待分析完成
4. 选择画质，点击 **开始下载**

**磁力/种子下载**：直接粘贴 magnet 链接或 `file://` 种子路径，程序自动识别并跳转 aria2c 引擎。

### CLI 模式

```bash
# 查看帮助
UniversalVideoDownloader -h

# 基础下载
UniversalVideoDownloader -c https://www.bilibili.com/video/BV1xx411c7XX

# 指定下载目录
UniversalVideoDownloader -c <URL> -o ~/Videos

# Cookie 注入
UniversalVideoDownloader -c <URL> -C firefox

# 合集模式
UniversalVideoDownloader -c <URL> -p

# 磁力下载
UniversalVideoDownloader -c "magnet:?xt=urn:btih:..."

# 详细日志
UniversalVideoDownloader -c <URL> -v
```

CLI 参数一览：

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-c`, `--cli` | 启用终端模式 | — |
| `-o`, `--output` | 下载目录 | `~/Downloads` |
| `-C`, `--cookie` | Cookie 来源 (`firefox`, `chrome`, `edge`, `safari`) | 无 |
| `-p`, `--playlist` | 合集/播放列表模式 | 关闭 |
| `-f`, `--format` | 画质指定 | `bestvideo+bestaudio/best` |
| `-v`, `--verbose` | 详细日志 | 关闭 |
| `-h`, `--help` | 显示帮助 | — |

---

## 项目结构

```
├── main.cpp                入口（CLI / GUI 路由）
├── mainwindow.h/cpp        主窗口 UI 与下载编排
├── cli_runner.h/cpp        命令行模式
├── download_utils.h/cpp    下载工具函数 + 进度解析器
├── url_extractor.h/cpp     网页 HTML 抓取 + 媒体 URL 提取
├── platformutils.h/cpp     平台抽象（路径查找、系统检测）
├── toolmanager.h/cpp       Windows 端工具自动下载 / 解压
├── thememanager.h/cpp      深色/亮色外观自适应
└── CMakeLists.txt          构建配置（Qt 6.5+ / C++20）
```

## 技术栈

- **语言**：C++20
- **UI**：Qt 6.5+（Widgets 模块，macOS 原生 Cocoa 风格）
- **外部引擎**：yt-dlp（视频解析下载） + FFmpeg（合并转码） + aria2c（BT/磁力）
- **网络**：QNetworkAccessManager（网页抓取 + Windows 工具下载）
- **构建**：CMake ≥ 3.16
- **CI/CD**：GitHub Actions

---

## 常见问题

**B站解析失败？** — 确保浏览器已登录 B站，Cookie 下拉选择正确浏览器，必要时关闭浏览器重试。

**下载无声音？** — 确认 FFmpeg 已安装（macOS: `brew install ffmpeg`，Windows: 放 `ffmpeg.exe` 到程序目录）。

**自动解析找不到链接？** — 部分网站视频由 JS 动态加载，静态抓取无法获取。程序会自动回退到 yt-dlp 直接解析。无需操作。

**磁力/BT 下载慢？** — 取决于 peer 可用性。初次连接 DHT 需等待。若超时失败，说明当前无做种者，可稍后重试。

**网络中断怎么办？** — 直接重新点击下载，程序自动从断点续传。最多自动重试 3 次。

---

## 许可证

MIT License
