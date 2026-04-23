#include "toolmanager.h"
#include "platformutils.h"
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>

ToolManager::ToolManager(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , currentDownload(nullptr)
    , downloading(false) {
    updatePaths();
}

ToolManager::~ToolManager() {
    if (currentDownload) {
        currentDownload->abort();
        currentDownload->deleteLater();
    }
}

void ToolManager::updatePaths() {
    ytdlpPath = PlatformUtils::findYtdlpPath();
    ffmpegPath = PlatformUtils::findFfmpegPath();
    aria2cPath = PlatformUtils::findAria2cPath();
}

QString ToolManager::getYtdlpPath() const { return ytdlpPath; }
QString ToolManager::getFfmpegPath() const { return ffmpegPath; }
QString ToolManager::getAria2cPath() const { return aria2cPath; }
bool ToolManager::isDownloading() const { return downloading; }

bool ToolManager::needsYtdlp() const {
    QString appDir = PlatformUtils::getAppDir();
    QString localPath = appDir + (PlatformUtils::isWindows() ? "/yt-dlp.exe" : "/yt-dlp");
    return !QFile::exists(localPath) && !QFile::exists(ytdlpPath);
}

bool ToolManager::needsFfmpeg() const {
    QString appDir = PlatformUtils::getAppDir();
    QString localPath = appDir + (PlatformUtils::isWindows() ? "/ffmpeg.exe" : "/ffmpeg");
    return !QFile::exists(localPath) && !QFile::exists(ffmpegPath);
}

bool ToolManager::needsAria2c() const {
    QString appDir = PlatformUtils::getAppDir();
    QString localPath = appDir + (PlatformUtils::isWindows() ? "/aria2c.exe" : "/aria2c");
    return !QFile::exists(localPath) && !QFile::exists(aria2cPath);
}

bool ToolManager::hasYtdlpInPath() const {
    return !QStandardPaths::findExecutable("yt-dlp").isEmpty();
}

bool ToolManager::hasFfmpegInPath() const {
    return !QStandardPaths::findExecutable("ffmpeg").isEmpty();
}

bool ToolManager::hasAria2cInPath() const {
    return !QStandardPaths::findExecutable("aria2c").isEmpty();
}

void ToolManager::finishQueue() {
    downloading = false;
    emit toolsReady();
    emit logMessage("🎉 所有组件已就绪，可以开始使用！");
}

void ToolManager::checkAndDownloadTools() {
    if (!PlatformUtils::isWindows()) {
        emit toolsReady();
        return;
    }

    QString appDir = PlatformUtils::getAppDir();
    bool localYtdlp = QFile::exists(appDir + "/yt-dlp.exe");
    bool localFfmpeg = QFile::exists(appDir + "/ffmpeg.exe");
    bool localAria2c = QFile::exists(appDir + "/aria2c.exe");

    bool pathYtdlp = hasYtdlpInPath();
    bool pathFfmpeg = hasFfmpegInPath();
    bool pathAria2c = hasAria2cInPath();

    if (localYtdlp)
        emit logMessage("✅ 程序目录已存在 yt-dlp.exe");
    else if (pathYtdlp)
        emit logMessage("✅ 在 PATH 中找到 yt-dlp: " + QStandardPaths::findExecutable("yt-dlp"));

    if (localFfmpeg)
        emit logMessage("✅ 程序目录已存在 ffmpeg.exe");
    else if (pathFfmpeg)
        emit logMessage("✅ 在 PATH 中找到 ffmpeg: " + QStandardPaths::findExecutable("ffmpeg"));

    if (localAria2c)
        emit logMessage("✅ 程序目录已存在 aria2c.exe");
    else if (pathAria2c)
        emit logMessage("✅ 在 PATH 中找到 aria2c: " + QStandardPaths::findExecutable("aria2c"));

    bool ytdlpOk = localYtdlp || pathYtdlp;
    bool ffmpegOk = localFfmpeg || pathFfmpeg;
    bool aria2cOk = localAria2c || pathAria2c;

    if (ytdlpOk && ffmpegOk && aria2cOk) {
        emit toolsReady();
        return;
    }

    // 缺少组件，询问用户
    QString missing;
    if (!ytdlpOk) missing += "yt-dlp";
    if (!ffmpegOk) { if (!missing.isEmpty()) missing += "、"; missing += "ffmpeg"; }
    if (!aria2cOk) { if (!missing.isEmpty()) missing += "、"; missing += "aria2c"; }

    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr, "缺少必要组件",
        QString("检测到系统中缺少 %1，是否自动下载到程序目录？\n\n下载位置：%2")
            .arg(missing, appDir),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        emit logMessage("⏳ 正在准备下载缺失的组件...");
        downloading = true;
        startDownloadQueue();
    } else {
        emit logMessage("⚠️ 跳过自动下载，请确保系统已安装 " + missing);
        emit toolsReady();
    }
}

void ToolManager::startDownloadQueue() {
    QString appDir = PlatformUtils::getAppDir();

    if (needsYtdlp()) {
        emit logMessage("📥 正在下载 yt-dlp...");
        downloadTool("yt-dlp",
                     "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe",
                     appDir + "/yt-dlp.exe");
    } else if (needsFfmpeg()) {
        emit logMessage("📥 正在下载 ffmpeg...");
        downloadTool("ffmpeg",
                     "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip",
                     appDir + "/ffmpeg.zip");
    } else if (needsAria2c()) {
        emit logMessage("📥 正在下载 aria2c...");
        downloadTool("aria2c",
                     "https://github.com/aria2/aria2/releases/download/release-1.37.0/aria2-1.37.0-win-64bit-build1.zip",
                     appDir + "/aria2c.zip");
    }
}

void ToolManager::downloadTool(const QString &toolName, const QString &url, const QString &savePath) {
    try {
        pendingToolName = toolName;
        pendingSavePath = savePath;

        QNetworkRequest request{QUrl(url)};
        request.setTransferTimeout(300000);

        currentDownload = networkManager->get(request);

        connect(currentDownload, &QNetworkReply::downloadProgress,
                this, &ToolManager::handleDownloadProgress);
        connect(currentDownload, &QNetworkReply::finished,
                this, &ToolManager::handleDownloadFinished);
        connect(currentDownload, &QNetworkReply::errorOccurred,
                this, &ToolManager::handleDownloadError);
    } catch (const std::exception &e) {
        emit downloadError(toolName, QString("启动下载失败: %1").arg(e.what()));
        finishQueue();
    }
}

void ToolManager::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesTotal > 0) {
        int percent = static_cast<int>((bytesReceived * 100) / bytesTotal);
        emit downloadProgress(pendingToolName, percent);
        emit logMessage(QString("📥 下载 %1: %2% (%3 KB)")
                            .arg(pendingToolName)
                            .arg(percent)
                            .arg(bytesReceived / 1024));
    }
}

void ToolManager::handleDownloadFinished() {
    if (!currentDownload) return;

    if (currentDownload->error() != QNetworkReply::NoError) {
        emit downloadError(pendingToolName, currentDownload->errorString());
        currentDownload->deleteLater();
        currentDownload = nullptr;
        finishQueue();
        return;
    }

    try {
        QByteArray data = currentDownload->readAll();
        QFile file(pendingSavePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit downloadError(pendingToolName, "无法保存文件: " + file.errorString());
            currentDownload->deleteLater();
            currentDownload = nullptr;
            finishQueue();
            return;
        }

        file.write(data);
        file.close();
        emit logMessage("✅ " + pendingToolName + " 下载完成！");

        currentDownload->deleteLater();
        currentDownload = nullptr;

        // 链式下载：yt-dlp → ffmpeg → aria2c
        if (pendingToolName == "ffmpeg") {
            extractFfmpeg(pendingSavePath);
        } else if (pendingToolName == "aria2c") {
            extractAria2c(pendingSavePath);
        } else {
            // yt-dlp 下载完成
            updatePaths();
            if (needsFfmpeg()) {
                emit logMessage("📥 正在下载 ffmpeg...");
                downloadTool("ffmpeg",
                             "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip",
                             PlatformUtils::getAppDir() + "/ffmpeg.zip");
            } else if (needsAria2c()) {
                emit logMessage("📥 正在下载 aria2c...");
                downloadTool("aria2c",
                             "https://github.com/aria2/aria2/releases/download/release-1.37.0/aria2-1.37.0-win-64bit-build1.zip",
                             PlatformUtils::getAppDir() + "/aria2c.zip");
            } else {
                finishQueue();
            }
        }
    } catch (const std::exception &e) {
        emit downloadError(pendingToolName, QString("处理下载文件失败: %1").arg(e.what()));
        finishQueue();
    }
}

void ToolManager::handleDownloadError(QNetworkReply::NetworkError error) {
    Q_UNUSED(error)
    QString errorMsg = currentDownload ? currentDownload->errorString() : "未知错误";
    emit downloadError(pendingToolName, errorMsg);
}

void ToolManager::extractFfmpeg(const QString &zipPath) {
    try {
        emit logMessage("⏳ 正在解压 ffmpeg...");
        QString appDir = PlatformUtils::getAppDir();

        QProcess *extractProcess = new QProcess(this);
        extractProcess->setWorkingDirectory(appDir);
        extractProcess->start("powershell", QStringList()
            << "-Command"
            << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                   .arg(zipPath, appDir));

        connect(extractProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, extractProcess, appDir, zipPath](int exitCode, QProcess::ExitStatus) {
            try {
                if (exitCode == 0) {
                    QDir subDir(appDir + "/ffmpeg-master-latest-win64-gpl/bin");
                    if (subDir.exists() && QFile::exists(subDir.absoluteFilePath("ffmpeg.exe"))) {
                        QString targetPath = appDir + "/ffmpeg.exe";
                        QFile::remove(targetPath);
                        if (QFile::copy(subDir.absoluteFilePath("ffmpeg.exe"), targetPath)) {
                            emit logMessage("✅ ffmpeg 解压完成！");
                        } else {
                            emit logMessage("⚠️ ffmpeg 文件复制失败，请手动复制。");
                        }
                        QDir(appDir + "/ffmpeg-master-latest-win64-gpl").removeRecursively();
                    } else {
                        emit logMessage("⚠️ 未找到 ffmpeg.exe，请手动解压。");
                    }
                    QFile::remove(zipPath);
                } else {
                    emit logMessage("❌ ffmpeg 解压失败，请手动解压。");
                }
            } catch (const std::exception &e) {
                emit logMessage(QString("❌ 解压过程出错: %1").arg(e.what()));
            }

            extractProcess->deleteLater();
            updatePaths();

            // 继续下载 aria2c
            if (needsAria2c()) {
                emit logMessage("📥 正在下载 aria2c...");
                downloadTool("aria2c",
                             "https://github.com/aria2/aria2/releases/download/release-1.37.0/aria2-1.37.0-win-64bit-build1.zip",
                             PlatformUtils::getAppDir() + "/aria2c.zip");
            } else {
                finishQueue();
            }
        });
    } catch (const std::exception &e) {
        emit downloadError("ffmpeg", QString("启动解压失败: %1").arg(e.what()));
        finishQueue();
    }
}

void ToolManager::extractAria2c(const QString &zipPath) {
    try {
        emit logMessage("⏳ 正在解压 aria2c...");
        QString appDir = PlatformUtils::getAppDir();

        QProcess *extractProcess = new QProcess(this);
        extractProcess->setWorkingDirectory(appDir);
        extractProcess->start("powershell", QStringList()
            << "-Command"
            << QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                   .arg(zipPath, appDir));

        connect(extractProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, extractProcess, appDir, zipPath](int exitCode, QProcess::ExitStatus) {
            try {
                if (exitCode == 0) {
                    // aria2c zip 解压后直接在根目录有 aria2c.exe
                    QDir extractedDir(appDir);
                    // 查找解压出的 aria2c 目录（如 aria2-1.37.0-win-64bit-build1）
                    QStringList dirs = extractedDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    for (const QString &dir : dirs) {
                        if (dir.startsWith("aria2")) {
                            QString srcPath = appDir + "/" + dir + "/aria2c.exe";
                            if (QFile::exists(srcPath)) {
                                QString targetPath = appDir + "/aria2c.exe";
                                QFile::remove(targetPath);
                                if (QFile::copy(srcPath, targetPath)) {
                                    emit logMessage("✅ aria2c 解压完成！");
                                } else {
                                    emit logMessage("⚠️ aria2c 文件复制失败，请手动复制。");
                                }
                                QDir(appDir + "/" + dir).removeRecursively();
                                break;
                            }
                        }
                    }
                    QFile::remove(zipPath);
                } else {
                    emit logMessage("❌ aria2c 解压失败，请手动解压。");
                }
            } catch (const std::exception &e) {
                emit logMessage(QString("❌ 解压过程出错: %1").arg(e.what()));
            }

            extractProcess->deleteLater();
            updatePaths();
            finishQueue();
        });
    } catch (const std::exception &e) {
        emit downloadError("aria2c", QString("启动解压失败: %1").arg(e.what()));
        finishQueue();
    }
}
