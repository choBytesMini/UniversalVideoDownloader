#ifndef TOOLMANAGER_H
#define TOOLMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

class ToolManager : public QObject {
    Q_OBJECT

public:
    explicit ToolManager(QObject *parent = nullptr);
    ~ToolManager();

    // 检查并下载缺失的工具
    void checkAndDownloadTools();

    // 获取当前 yt-dlp 路径
    QString getYtdlpPath() const;

    // 获取当前 ffmpeg 路径
    QString getFfmpegPath() const;

    // 是否正在下载
    bool isDownloading() const;

signals:
    // 下载进度更新
    void downloadProgress(const QString &toolName, int percent);

    // 日志消息
    void logMessage(const QString &msg);

    // 所有工具就绪
    void toolsReady();

    // 下载出错
    void downloadError(const QString &toolName, const QString &error);

private slots:
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleDownloadFinished();
    void handleDownloadError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *networkManager;
    QNetworkReply *currentDownload;

    QString ytdlpPath;
    QString ffmpegPath;
    QString pendingToolName;
    QString pendingSavePath;
    bool downloading;

    bool needsYtdlp() const;
    bool needsFfmpeg() const;
    void downloadTool(const QString &toolName, const QString &url, const QString &savePath);
    void startDownloadQueue();
    void extractFfmpeg(const QString &zipPath);
    void updatePaths();
};

#endif // TOOLMANAGER_H
