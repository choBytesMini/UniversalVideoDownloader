#ifndef PLATFORMUTILS_H
#define PLATFORMUTILS_H

#include <QString>

class PlatformUtils {
public:
    // 获取 yt-dlp 可执行文件路径
    static QString findYtdlpPath();

    // 获取 ffmpeg 可执行文件路径
    static QString findFfmpegPath();

    // 获取 aria2c 可执行文件路径
    static QString findAria2cPath();

    // 获取程序所在目录
    static QString getAppDir();

    // 检查是否为 Windows 平台
    static bool isWindows();

    // 检查是否为 macOS 平台
    static bool isMacOS();
};

#endif // PLATFORMUTILS_H
