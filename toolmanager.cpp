#include "toolmanager.h"
#include "platformutils.h"
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QProcess>

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
}

QString ToolManager::getYtdlpPath() const {
    return ytdlpPath;
}

QString ToolManager::getFfmpegPath() const {
    return ffmpegPath;
}

bool ToolManager::isDownloading() const {
    return downloading;
}

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

void ToolManager::checkAndDownloadTools() {
    // 仅在 Windows 平台自动下载
    if (!PlatformUtils::isWindows()) {
        emit toolsReady();
        return;
    }

    bool needYtdlp = needsYtdlp();
    bool needFfmpeg = needsFfmpeg();

    if (!needYtdlp && !needFfmpeg) {
        emit toolsReady();
        return;
    }

    // 提示用户
    QString missing;
    if (needYtdlp) missing += "yt-dlp";
    if (needFfmpeg) {
        if (!missing.isEmpty()) missing += " 和 ";
        missing += "ffmpeg";
    }

    QString appDir = PlatformUtils::getAppDir();
    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr,
        "缺少必要组件",
        QString("检测到程序目录下缺少 %1，是否自动下载？\n\n"
                "下载位置：%2")
            .arg(missing, appDir),
        QMessageBox::Yes | QMessageBox::No
    );

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
    }
}

void ToolManager::downloadTool(const QString &toolName, const QString &url, const QString &savePath) {
    try {
        pendingToolName = toolName;
        pendingSavePath = savePath;

        QNetworkRequest request{QUrl(url)};
        request.setTransferTimeout(300000); // 5分钟超时

        currentDownload = networkManager->get(request);

        connect(currentDownload, &QNetworkReply::downloadProgress,
                this, &ToolManager::handleDownloadProgress);
        connect(currentDownload, &QNetworkReply::finished,
                this, &ToolManager::handleDownloadFinished);
        connect(currentDownload, &QNetworkReply::errorOccurred,
                this, &ToolManager::handleDownloadError);
    } catch (const std::exception &e) {
        emit downloadError(toolName, QString("启动下载失败: %1").arg(e.what()));
        downloading = false;
        emit toolsReady();
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
        downloading = false;
        emit toolsReady();
        return;
    }

    try {
        // 保存文件
        QByteArray data = currentDownload->readAll();
        QFile file(pendingSavePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit downloadError(pendingToolName, "无法保存文件: " + file.errorString());
            currentDownload->deleteLater();
            currentDownload = nullptr;
            downloading = false;
            emit toolsReady();
            return;
        }

        file.write(data);
        file.close();
        emit logMessage("✅ " + pendingToolName + " 下载完成！");

        currentDownload->deleteLater();
        currentDownload = nullptr;

        // 如果下载的是 ffmpeg.zip，需要解压
        if (pendingToolName == "ffmpeg") {
            extractFfmpeg(pendingSavePath);
        } else {
            // yt-dlp 下载完成
            updatePaths();

            // 检查是否还需要下载 ffmpeg
            if (needsFfmpeg()) {
                emit logMessage("📥 正在下载 ffmpeg...");
                downloadTool("ffmpeg",
                             "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip",
                             PlatformUtils::getAppDir() + "/ffmpeg.zip");
            } else {
                downloading = false;
                emit toolsReady();
                emit logMessage("🎉 所有组件已就绪，可以开始使用！");
            }
        }
    } catch (const std::exception &e) {
        emit downloadError(pendingToolName, QString("处理下载文件失败: %1").arg(e.what()));
        downloading = false;
        emit toolsReady();
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
                this, [this, extractProcess, appDir](int exitCode, QProcess::ExitStatus) {
            try {
                if (exitCode == 0) {
                    // 从解压的文件夹中找到 ffmpeg.exe 并移动到程序目录
                    QDir subDir(appDir + "/ffmpeg-master-latest-win64-gpl/bin");
                    if (subDir.exists() && QFile::exists(subDir.absoluteFilePath("ffmpeg.exe"))) {
                        QString targetPath = appDir + "/ffmpeg.exe";
                        QFile::remove(targetPath); // 先删除已存在的文件
                        if (QFile::copy(subDir.absoluteFilePath("ffmpeg.exe"), targetPath)) {
                            emit logMessage("✅ ffmpeg 解压完成！");
                        } else {
                            emit logMessage("⚠️ ffmpeg 文件复制失败，请手动复制。");
                        }
                        // 清理解压的文件夹
                        QDir(appDir + "/ffmpeg-master-latest-win64-gpl").removeRecursively();
                    } else {
                        emit logMessage("⚠️ 未找到 ffmpeg.exe，请手动解压。");
                    }

                    // 删除zip文件
                    QFile::remove(pendingSavePath);
                } else {
                    emit logMessage("❌ ffmpeg 解压失败，请手动解压。");
                }
            } catch (const std::exception &e) {
                emit logMessage(QString("❌ 解压过程出错: %1").arg(e.what()));
            }

            extractProcess->deleteLater();
            updatePaths();
            downloading = false;
            emit toolsReady();
            emit logMessage("🎉 所有组件已就绪，可以开始使用！");
        });
    } catch (const std::exception &e) {
        emit downloadError("ffmpeg", QString("启动解压失败: %1").arg(e.what()));
        downloading = false;
        emit toolsReady();
    }
}
