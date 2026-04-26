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

class QLabel;

class ThemeManager;
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
    void handleAria2cError();
    void handleAria2cFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // URL 自动解析回调
    void onUrlsExtracted(const QStringList &extractedUrls);
    void onUrlExtractFailed(const QString &pageUrl);
    void onUrlExtractError(const QString &errorMsg);

private slots:
    void onVideoSelectionChanged(int index);

private:
    // UI 控件
    QLineEdit *urlInput;
    QLineEdit *nameInput;
    QPushButton *analyzeBtn;
    QPushButton *downloadBtn;
    QPushButton *stopBtn;
    QLabel *videoLabel;
    QComboBox *videoBox;
    QComboBox *resolutionBox;
    QComboBox *cookieBox;
    QCheckBox *playlistCheckBox;
    QCheckBox *autoExtractCheckBox;
    QTextEdit *logConsole;
    QProgressBar *progressBar;
    QLabel *statusLabel;  // 下载详情：速度/已下载/剩余时间

    // QProcess 实例
    QProcess *analyzeProcess;
    QProcess *downloadProcess;
    QProcess *aria2cProcess;

    // 状态
    QMap<QString, QString> formatMap;
    QMap<QString, QString> videoMap;
    QString currentUrl;
    QString ytdlpPath;
    QString ffmpegPath;
    QString aria2cPath;

    // 断网重试
    int retryCount = 0;
    static const int MAX_RETRIES = 3;
    bool usingAria2c = false; // 当前下载是否使用 aria2c

    ThemeManager *themeManager;
    ToolManager *toolManager;
    UrlExtractor *urlExtractor;

    void logMessage(const QString &msg);
    void setupToolManager();
    void startYtdlpAnalyze();
    void startYtdlpDownload();
    void startAria2cDownload(const QString &url);
    void retryDownload();
    void setDownloadUIEnabled(bool enabled);
    void updateProgressDetail(int percent, const QString &size,
                              const QString &speed, const QString &eta);
};

#endif // MAINWINDOW_H
