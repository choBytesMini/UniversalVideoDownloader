#include "platformutils.h"
#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>

QString PlatformUtils::findYtdlpPath() {
#ifdef Q_OS_WIN
    // 优先查找程序所在目录
    QString localPath = getAppDir() + "/yt-dlp.exe";
    if (QFile::exists(localPath)) {
        return localPath;
    }
    // 回退到系统环境变量 PATH
    QString pathExe = QStandardPaths::findExecutable("yt-dlp");
    if (!pathExe.isEmpty()) {
        return pathExe;
    }
    return QString(); // 未找到
#elif defined(Q_OS_MACOS)
    QString localPath = getAppDir() + "/yt-dlp";
    if (QFile::exists(localPath)) return localPath;

    QStringList macPaths = {"/opt/homebrew/bin/yt-dlp", "/usr/local/bin/yt-dlp", "/opt/local/bin/yt-dlp"};
    for (const QString& p : macPaths) {
        if (QFile::exists(p)) return p;
    }
    QString pathExe = QStandardPaths::findExecutable("yt-dlp");
    if (!pathExe.isEmpty()) return pathExe;
    
    return "yt-dlp"; // 回退到系统 PATH
#else
    return "yt-dlp";
#endif
}

QString PlatformUtils::findFfmpegPath() {
#ifdef Q_OS_WIN
    // 优先查找程序所在目录
    QString localPath = getAppDir() + "/ffmpeg.exe";
    if (QFile::exists(localPath)) {
        return localPath;
    }
    // 回退到系统环境变量 PATH
    QString pathExe = QStandardPaths::findExecutable("ffmpeg");
    if (!pathExe.isEmpty()) {
        return pathExe;
    }
    return QString(); // 未找到
#elif defined(Q_OS_MACOS)
    QString localPath = getAppDir() + "/ffmpeg";
    if (QFile::exists(localPath)) return localPath;

    QStringList macPaths = {"/opt/homebrew/bin/ffmpeg", "/usr/local/bin/ffmpeg", "/opt/local/bin/ffmpeg"};
    for (const QString& p : macPaths) {
        if (QFile::exists(p)) return p;
    }
    QString pathExe = QStandardPaths::findExecutable("ffmpeg");
    if (!pathExe.isEmpty()) return pathExe;
    
    return QString(); // 未找到
#else
    return QString();
#endif
}

QString PlatformUtils::findAria2cPath() {
#ifdef Q_OS_WIN
    // 优先查找程序所在目录
    QString localPath = getAppDir() + "/aria2c.exe";
    if (QFile::exists(localPath)) {
        return localPath;
    }
    // 回退到系统环境变量 PATH
    QString pathExe = QStandardPaths::findExecutable("aria2c");
    if (!pathExe.isEmpty()) {
        return pathExe;
    }
    return QString(); // 未找到
#elif defined(Q_OS_MACOS)
    QString localPath = getAppDir() + "/aria2c";
    if (QFile::exists(localPath)) return localPath;

    QStringList macPaths = {"/opt/homebrew/bin/aria2c", "/usr/local/bin/aria2c", "/opt/local/bin/aria2c"};
    for (const QString& p : macPaths) {
        if (QFile::exists(p)) return p;
    }
    QString pathExe = QStandardPaths::findExecutable("aria2c");
    if (!pathExe.isEmpty()) return pathExe;
    
    return "aria2c"; // 回退到系统 PATH
#else
    return "aria2c";
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
