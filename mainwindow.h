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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalyzeClicked();
    void onDownloadClicked();
    
    // QProcess 的异步回调
    void handleAnalyzeFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleDownloadOutput();
    void handleDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProcessError(QProcess::ProcessError error);

private:
    QLineEdit *urlInput;
    QLineEdit *nameInput;
    QPushButton *analyzeBtn;
    QPushButton *downloadBtn;
    QComboBox *resolutionBox;
    QComboBox *cookieBox;
    QCheckBox *playlistCheckBox;
    QTextEdit *logConsole;
    QProgressBar *progressBar;

    QProcess *analyzeProcess;
    QProcess *downloadProcess;

    QMap<QString, QString> formatMap; // 存储 "分辨率显示名称" -> "底层 format_id"
    QString currentUrl;
    QString ytdlpPath;

    void logMessage(const QString &msg);
    void findYtDlpPath();
};

#endif // MAINWINDOW_H
