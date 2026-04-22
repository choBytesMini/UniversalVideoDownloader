#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("全能网页视频下载器 (支持 Cookie 绕过)");
    resize(700, 500);

    // 跨平台识别底层的 yt-dlp 执行文件路径
#ifdef Q_OS_WIN
    ytdlpPath = "yt-dlp.exe"; 
#else
    ytdlpPath = "/opt/homebrew/bin/yt-dlp";
    if (!QFile::exists(ytdlpPath)) {
        ytdlpPath = "yt-dlp"; 
    }
#endif

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 第一行：URL、Cookie 来源 与 解析按钮
    QHBoxLayout *urlLayout = new QHBoxLayout();
    urlLayout->addWidget(new QLabel("网页地址:"));
    urlInput = new QLineEdit();
    urlLayout->addWidget(urlInput);
    
    urlLayout->addWidget(new QLabel("使用 Cookie:"));
    cookieBox = new QComboBox();
    cookieBox->addItems({"无", "firefox", "chrome", "edge", "safari"});
    cookieBox->setCurrentText("firefox");
    urlLayout->addWidget(cookieBox);

    // --- 合集模式复选框 ---
    playlistCheckBox = new QCheckBox("合集模式");
    urlLayout->addWidget(playlistCheckBox);

    analyzeBtn = new QPushButton("🔍 解析视频");
    urlLayout->addWidget(analyzeBtn);
    mainLayout->addLayout(urlLayout);

    // 第二行：画质选择、文件名、下载按钮
    QHBoxLayout *downloadLayout = new QHBoxLayout();
    downloadLayout->addWidget(new QLabel("选择画质:"));
    resolutionBox = new QComboBox();
    resolutionBox->setMinimumWidth(180);
    downloadLayout->addWidget(resolutionBox);
    
    downloadLayout->addWidget(new QLabel("文件名:"));
    nameInput = new QLineEdit("video.mp4");
    downloadLayout->addWidget(nameInput);
    
    downloadBtn = new QPushButton("⬇️ 开始下载");
    downloadBtn->setEnabled(false);
    downloadLayout->addWidget(downloadBtn);
    mainLayout->addLayout(downloadLayout);

    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    mainLayout->addWidget(progressBar);

    logConsole = new QTextEdit();
    logConsole->setReadOnly(true);
    logConsole->setStyleSheet("background-color: #1e1e1e; color: #d4d4d4; font-family: 'Menlo';");
    mainLayout->addWidget(logConsole);

    setCentralWidget(centralWidget);

    analyzeProcess = new QProcess(this);
    downloadProcess = new QProcess(this);

    connect(analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    connect(downloadBtn, &QPushButton::clicked, this, &MainWindow::onDownloadClicked);
    
    connect(analyzeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::handleAnalyzeFinished);
    connect(analyzeProcess, &QProcess::errorOccurred, this, &MainWindow::handleProcessError);
    
    connect(downloadProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::handleDownloadOutput);
    connect(downloadProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::handleDownloadFinished);
    connect(downloadProcess, &QProcess::errorOccurred, this, &MainWindow::handleProcessError);
}

MainWindow::~MainWindow() {}

void MainWindow::logMessage(const QString &msg) {
    logConsole->append(msg);
}

void MainWindow::onAnalyzeClicked() {
    currentUrl = urlInput->text().trimmed();
    if (currentUrl.isEmpty()) {
        logMessage("❌ 请先输入链接");
        return;
    }

    logMessage("⏳ 正在尝试解析网页数据 (包含 Cookie 注入)...");
    analyzeBtn->setEnabled(false);
    downloadBtn->setEnabled(false);
    resolutionBox->clear();
    formatMap.clear();

    QStringList args;
    QString browser = cookieBox->currentText();
    if (browser != "无") {
        args << "--cookies-from-browser" << browser;
    }

    // --- 合集参数分流 ---
    if (playlistCheckBox->isChecked()) {
        // 如果是合集，开启合集解析，并使用 flat-playlist 加快 JSON 返回速度
        args << "--yes-playlist" << "--flat-playlist";
    } else {
        // 正常模式：拒绝合集，防止误下整个播放列表
        args << "--no-playlist";
    }
    // -------------------------

    args << "-J" << currentUrl;
    analyzeProcess->start(ytdlpPath, args);
}

void MainWindow::handleAnalyzeFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    analyzeBtn->setEnabled(true);
    if (exitCode != 0) {
        logMessage("❌ 解析失败。若解析B站，请确保浏览器已登录且在列表中选对浏览器。");
        logMessage(QString::fromUtf8(analyzeProcess->readAllStandardError()));
        return;
    }

    QByteArray jsonData = analyzeProcess->readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    QJsonObject root = doc.object();
    QString title = root["title"].toString();

    // --- 处理合集模式的 UI 表现 ---
    if (playlistCheckBox->isChecked()) {
        logMessage("✅ 识别到合集/列表: " + title);
        logMessage("⚠️ 合集模式下，将自动为您下载所有分P的最佳画质。");
        
        resolutionBox->clear();
        formatMap.clear();
        formatMap.insert("自动最佳合集画质", "bestvideo+bestaudio/best");
        resolutionBox->addItem("自动最佳合集画质");
        
        // 改变文件名输入框，提醒用户合集会自动编号
        nameInput->setText("合集自动编号下载");
        nameInput->setEnabled(false); // 禁用输入框防止乱改导致覆盖
        
        downloadBtn->setEnabled(true);
        return; // 提前结束，跳过后续的单集画质扫描
    }
    // ---------------------------------
    
    // 恢复输入框可用状态
    nameInput->setEnabled(true);
    logMessage("✅ 提取成功: " + title);
    if (nameInput->text() == "video.mp4" || nameInput->text() == "合集自动编号下载") {
        nameInput->setText(title + ".mp4");
    } 

    QJsonArray formats = root["formats"].toArray();
    for (const QJsonValue &val : formats) {
        QJsonObject fmt = val.toObject();
        QString formatId = fmt["format_id"].toString();
        QString ext = fmt["ext"].toString();
        
        // 鲁棒性提取：尝试 resolution -> height -> format_id
        QString resStr = fmt["resolution"].toString();
        if (resStr.isEmpty()) {
            int h = fmt["height"].toInt(0);
            resStr = (h > 0) ? QString::number(h) + "p" : "默认流 " + formatId;
        }

        // 屏蔽纯音频
        if (resStr == "audio only" || fmt["vcodec"].toString() == "none") continue;

        QString displayName = QString("%1 (%2)").arg(resStr).arg(ext);
        QString bestFormat = formatId;
        if (fmt["acodec"].toString() == "none") bestFormat += "+bestaudio/best";

        if (!formatMap.contains(displayName)) {
            formatMap.insert(displayName, bestFormat);
            resolutionBox->addItem(displayName);
        }
    }

    if (resolutionBox->count() > 0) {
        formatMap.insert("自动最佳画质", "bestvideo+bestaudio/best");
        resolutionBox->insertItem(0, "自动最佳画质");
        resolutionBox->setCurrentIndex(0);
        downloadBtn->setEnabled(true);
        logMessage("✨ 请选择画质并开始下载。");
    }
}

void MainWindow::onDownloadClicked() {
    QString formatId = formatMap[resolutionBox->currentText()];
    QString outName = nameInput->text();
    
    // 权限修复：强制使用系统的“下载”文件夹
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    downloadProcess->setWorkingDirectory(downloadDir);

    logMessage("🚀 开始下载，保存至: " + downloadDir);
    downloadBtn->setEnabled(false);
    analyzeBtn->setEnabled(false);
    progressBar->setValue(0);

    QStringList args;
    QString browser = cookieBox->currentText();
    if (browser != "无") {
        args << "--cookies-from-browser" << browser;
    }
     // --- 新增：跨平台修复 ffmpeg 路径丢失问题 ---
    #ifdef Q_OS_WIN
      // Windows 端：自动获取当前 exe 所在目录，并寻找配套的 ffmpeg.exe
      QString ffmpegPath = QCoreApplication::applicationDirPath() + "/ffmpeg.exe";
      if (QFile::exists(ffmpegPath)) {
        args << "--ffmpeg-location" << ffmpegPath;
      }
    #else
      // macOS 端：强制指定 Homebrew 安装的 ffmpeg 路径
      if (QFile::exists("/opt/homebrew/bin/ffmpeg")) {
        args << "--ffmpeg-location" << "/opt/homebrew/bin/ffmpeg";
      }
    #endif
    // ---------------------------------------------    
    args << "-f" << formatId << "--merge-output-format" << "mp4" << "--newline" << "-o" << outName << currentUrl;
    if (playlistCheckBox->isChecked()) {
        args << "--yes-playlist";
        args << "-o" << "%(playlist_index)02d_%(title)s.%(ext)s"; 
    } else {
        args << "--no-playlist";
        args << "-o" << outName; 
    }

    args << currentUrl;
    downloadProcess->start(ytdlpPath, args);
}

void MainWindow::handleDownloadOutput() {
    QString output = QString::fromUtf8(downloadProcess->readAllStandardOutput());
    QRegularExpression re("\\[download\\]\\s+([0-9.]+)\\%");
    QRegularExpressionMatch match = re.match(output);
    if (match.hasMatch()) {
        progressBar->setValue(static_cast<int>(match.captured(1).toDouble()));
    }
}

void MainWindow::handleDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    downloadBtn->setEnabled(true);
    analyzeBtn->setEnabled(true);
    if (exitCode == 0) {
        progressBar->setValue(100);
        logMessage("🎉 下载任务圆满完成！");
    } else {
        logMessage("❌ 下载出错，请检查网络或是否需要更新 yt-dlp。");
    }
}

void MainWindow::handleProcessError(QProcess::ProcessError error) {
    logMessage("❌ 引擎启动失败，请确保系统已安装 yt-dlp 和 ffmpeg。");
    analyzeBtn->setEnabled(true);
}
