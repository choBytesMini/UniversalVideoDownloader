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

    // 获取当前工具路径
    QString getYtdlpPath() const;
    QString getFfmpegPath() const;
    QString getAria2cPath() const;

    // 是否正在下载
    bool isDownloading() const;

signals:
    void downloadProgress(const QString &toolName, int percent);
    void logMessage(const QString &msg);
    void toolsReady();
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
    QString aria2cPath;
    QString pendingToolName;
    QString pendingSavePath;
    bool downloading;

    bool needsYtdlp() const;
    bool needsFfmpeg() const;
    bool needsAria2c() const;
    bool hasYtdlpInPath() const;
    bool hasFfmpegInPath() const;
    bool hasAria2cInPath() const;
    void downloadTool(const QString &toolName, const QString &url, const QString &savePath);
    void startDownloadQueue();
    void extractFfmpeg(const QString &zipPath);
    void extractAria2c(const QString &zipPath);
    void updatePaths();
    void finishQueue();
};

#endif // TOOLMANAGER_H
