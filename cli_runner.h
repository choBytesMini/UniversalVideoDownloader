#ifndef CLI_RUNNER_H
#define CLI_RUNNER_H

#include <QObject>
#include <QProcess>
#include <QString>

class QCoreApplication;

class CliRunner : public QObject {
    Q_OBJECT

public:
    explicit CliRunner(QObject *parent = nullptr);

    // 解析参数并运行，返回退出码
    int run(int argc, char *argv[]);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess *process;
    QString currentUrl;
    QString outputDir;
    QString cookieBrowser;
    QString ytdlpPath;
    QString aria2cPath;
    QString ffmpegPath;
    bool isMagnet;
    bool verbose;

    void printUsage();
    void printHeader();
    void printProgress(const QString &line);
    void startDownload();

    // ANSI 颜色
    static const char *RESET;
    static const char *BOLD;
    static const char *GREEN;
    static const char *YELLOW;
    static const char *RED;
    static const char *CYAN;
    static const char *DIM;
};

#endif // CLI_RUNNER_H
