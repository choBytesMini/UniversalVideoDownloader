#include "platformutils.h"
#include <QCoreApplication>
#include <QFile>

QString PlatformUtils::findYtdlpPath() {
#ifdef Q_OS_WIN
    // Windows: 优先查找程序所在目录
    QString localPath = getAppDir() + "/yt-dlp.exe";
    if (QFile::exists(localPath)) {
        return localPath;
    }
    return "yt-dlp.exe"; // 回退到系统 PATH
#elif defined(Q_OS_MACOS)
    // macOS: 优先查找 Homebrew 安装路径
    QString homebrewPath = "/opt/homebrew/bin/yt-dlp";
    if (QFile::exists(homebrewPath)) {
        return homebrewPath;
    }
    return "yt-dlp"; // 回退到系统 PATH
#else
    return "yt-dlp";
#endif
}

QString PlatformUtils::findFfmpegPath() {
#ifdef Q_OS_WIN
    // Windows: 优先查找程序所在目录
    QString localPath = getAppDir() + "/ffmpeg.exe";
    if (QFile::exists(localPath)) {
        return localPath;
    }
    return QString(); // 未找到
#elif defined(Q_OS_MACOS)
    // macOS: 优先查找 Homebrew 安装路径
    QString homebrewPath = "/opt/homebrew/bin/ffmpeg";
    if (QFile::exists(homebrewPath)) {
        return homebrewPath;
    }
    return QString(); // 未找到
#else
    return QString();
#endif
}

QString PlatformUtils::getAppDir() {
    return QCoreApplication::applicationDirPath();
}

bool PlatformUtils::isWindows() {
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool PlatformUtils::isMacOS() {
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}
