#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QComboBox>
#include <QProcess>
#include <QMap>
#include <QCheckBox>

class ToolManager;
class UrlExtractor;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalyzeClicked();
    void onDownloadClicked();
    void onStopClicked();

    // yt-dlp QProcess 回调
    void handleAnalyzeFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleDownloadOutput();
    void handleDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProcessError(QProcess::ProcessError error);

    // aria2c QProcess 回调
    void handleAria2cOutput();
    void handleAria2cFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // URL 自动解析回调
    void onUrlExtracted(const QString &extractedUrl);
    void onUrlExtractFailed(const QString &pageUrl);
    void onUrlExtractError(const QString &errorMsg);

private:
    // UI 控件
    QLineEdit *urlInput;
    QLineEdit *nameInput;
    QPushButton *analyzeBtn;
    QPushButton *downloadBtn;
    QPushButton *stopBtn;
    QComboBox *resolutionBox;
    QComboBox *cookieBox;
    QCheckBox *playlistCheckBox;
    QCheckBox *autoExtractCheckBox;
    QTextEdit *logConsole;
    QProgressBar *progressBar;

    // QProcess 实例
    QProcess *analyzeProcess;
    QProcess *downloadProcess;
    QProcess *aria2cProcess;

    // 状态
    QMap<QString, QString> formatMap;
    QString currentUrl;
    QString ytdlpPath;
    QString ffmpegPath;
    QString aria2cPath;

    // 断网重试
    int retryCount = 0;
    static const int MAX_RETRIES = 3;
    bool usingAria2c = false; // 当前下载是否使用 aria2c

    ToolManager *toolManager;
    UrlExtractor *urlExtractor;

    void logMessage(const QString &msg);
    void setupToolManager();
    void startYtdlpAnalyze();
    void startYtdlpDownload();
    void startAria2cDownload(const QString &url);
    void retryDownload();
    void setDownloadUIEnabled(bool enabled);
    bool isMagnetOrTorrent(const QString &url) const;
};

#endif // MAINWINDOW_H
