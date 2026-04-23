#include "mainwindow.h"
#include "platformutils.h"
#include "toolmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("全能网页视频下载器 (支持 Cookie 绕过)");
    resize(700, 500);

    // 初始化路径
    ytdlpPath = PlatformUtils::findYtdlpPath();
    ffmpegPath = PlatformUtils::findFfmpegPath();

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
    logConsole->setStyleSheet(
        "background-color: #1e1e1e; color: #d4d4d4; font-family: 'Menlo';");
    mainLayout->addWidget(logConsole);

    setCentralWidget(centralWidget);

    analyzeProcess = new QProcess(this);
    downloadProcess = new QProcess(this);

    connect(analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    connect(downloadBtn, &QPushButton::clicked, this, &MainWindow::onDownloadClicked);

    connect(analyzeProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleAnalyzeFinished);
    connect(analyzeProcess, &QProcess::errorOccurred,
            this, &MainWindow::handleProcessError);

    connect(downloadProcess, &QProcess::readyReadStandardOutput,
            this, &MainWindow::handleDownloadOutput);
    connect(downloadProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleDownloadFinished);
    connect(downloadProcess, &QProcess::errorOccurred,
            this, &MainWindow::handleProcessError);

    // 初始化工具管理器
    setupToolManager();
}

MainWindow::~MainWindow() {}

void MainWindow::setupToolManager() {
    toolManager = new ToolManager(this);

    connect(toolManager, &ToolManager::logMessage,
            this, &MainWindow::logMessage);
    connect(toolManager, &ToolManager::toolsReady, this, [this]() {
        // 工具就绪后更新路径
        ytdlpPath = toolManager->getYtdlpPath();
        ffmpegPath = toolManager->getFfmpegPath();
        analyzeBtn->setEnabled(true);
        logMessage("🎉 所有组件已就绪，可以开始使用！");
    });
    connect(toolManager, &ToolManager::downloadError, this,
            [this](const QString &toolName, const QString &error) {
        logMessage(QString("❌ 下载 %1 失败: %2").arg(toolName, error));
    });
    connect(toolManager, &ToolManager::downloadProgress, this,
            [this](const QString &toolName, int percent) {
        progressBar->setValue(percent);
    });

    // 检查并下载缺失的工具
    if (PlatformUtils::isWindows()) {
        analyzeBtn->setEnabled(false);
        toolManager->checkAndDownloadTools();
    }
}

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
        args << "--yes-playlist" << "--flat-playlist";
    } else {
        args << "--no-playlist";
    }

    args << "-J" << currentUrl;
    analyzeProcess->start(ytdlpPath, args);
}

void MainWindow::handleAnalyzeFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus)

    analyzeBtn->setEnabled(true);
    if (exitCode != 0) {
        logMessage("❌ 解析失败。若解析B站，请确保浏览器已登录且在列表中选对浏览器。");
        logMessage(QString::fromUtf8(analyzeProcess->readAllStandardError()));
        return;
    }

    try {
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

            nameInput->setText("合集自动编号下载");
            nameInput->setEnabled(false);

            downloadBtn->setEnabled(true);
            return;
        }

        // 恢复输入框可用状态
        nameInput->setEnabled(true);
        logMessage("✅ 提取成功: " + title);
        nameInput->setText(title + ".mp4");

        QJsonArray formats = root["formats"].toArray();
        for (const QJsonValue &val : formats) {
            QJsonObject fmt = val.toObject();
            QString formatId = fmt["format_id"].toString();
            QString ext = fmt["ext"].toString();

            QString resStr = fmt["resolution"].toString();
            if (resStr.isEmpty()) {
                int h = fmt["height"].toInt(0);
                resStr = (h > 0) ? QString::number(h) + "p" : "默认流 " + formatId;
            }

            if (resStr == "audio only" || fmt["vcodec"].toString() == "none")
                continue;

            QString displayName = QString("%1 (%2)").arg(resStr, ext);
            QString bestFormat = formatId;
            if (fmt["acodec"].toString() == "none")
                bestFormat += "+bestaudio/best";

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
    } catch (const std::exception &e) {
        logMessage(QString("❌ 解析数据时出错: %1").arg(e.what()));
    } catch (...) {
        logMessage("❌ 解析数据时发生未知错误");
    }
}

void MainWindow::onDownloadClicked() {
    try {
        QString formatId = formatMap[resolutionBox->currentText()];
        QString outName = nameInput->text();

        if (outName.isEmpty()) {
            logMessage("❌ 请输入文件名");
            return;
        }

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

        // 跨平台 ffmpeg 路径
        if (!ffmpegPath.isEmpty() && QFile::exists(ffmpegPath)) {
            args << "--ffmpeg-location" << ffmpegPath;
        }

        args << "-f" << formatId << "--merge-output-format" << "mp4" << "--newline";

        if (playlistCheckBox->isChecked()) {
            args << "--yes-playlist";
            args << "-o" << "%(playlist_index)02d_%(title)s.%(ext)s";
        } else {
            args << "--no-playlist";
            args << "-o" << outName;
        }

        args << currentUrl;
        downloadProcess->start(ytdlpPath, args);
    } catch (const std::exception &e) {
        logMessage(QString("❌ 启动下载时出错: %1").arg(e.what()));
        downloadBtn->setEnabled(true);
        analyzeBtn->setEnabled(true);
    } catch (...) {
        logMessage("❌ 启动下载时发生未知错误");
        downloadBtn->setEnabled(true);
        analyzeBtn->setEnabled(true);
    }
}

void MainWindow::handleDownloadOutput() {
    try {
        QString output = QString::fromUtf8(downloadProcess->readAllStandardOutput());
        QRegularExpression re("\\[download\\]\\s+([0-9.]+)\\%");
        QRegularExpressionMatch match = re.match(output);
        if (match.hasMatch()) {
            progressBar->setValue(static_cast<int>(match.captured(1).toDouble()));
        }
    } catch (...) {
        // 忽略输出解析错误
    }
}

void MainWindow::handleDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus)

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
    Q_UNUSED(error)
    logMessage("❌ 引擎启动失败，请确保系统已安装 yt-dlp 和 ffmpeg。");
    analyzeBtn->setEnabled(true);
    downloadBtn->setEnabled(true);
}
