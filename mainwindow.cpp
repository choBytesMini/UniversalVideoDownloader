#include "mainwindow.h"
#include "platformutils.h"
#include "toolmanager.h"
#include "url_extractor.h"
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
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("全能网页视频下载器 (支持 Cookie 绕过)");
    resize(700, 500);

    // 初始化路径
    ytdlpPath = PlatformUtils::findYtdlpPath();
    ffmpegPath = PlatformUtils::findFfmpegPath();
    aria2cPath = PlatformUtils::findAria2cPath();

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 第一行：URL、Cookie 来源 与 解析按钮
    QHBoxLayout *urlLayout = new QHBoxLayout();
    urlLayout->addWidget(new QLabel("网页地址:"));
    urlInput = new QLineEdit();
    urlInput->setPlaceholderText("输入视频链接、magnet 链接或 .torrent 文件路径");
    urlLayout->addWidget(urlInput);

    urlLayout->addWidget(new QLabel("使用 Cookie:"));
    cookieBox = new QComboBox();
    cookieBox->addItems({"无", "firefox", "chrome", "edge", "safari"});
    cookieBox->setCurrentText("firefox");
    urlLayout->addWidget(cookieBox);

    // --- 合集模式复选框 ---
    playlistCheckBox = new QCheckBox("合集模式");
    urlLayout->addWidget(playlistCheckBox);

    // --- 自动解析 URL 复选框 ---
    autoExtractCheckBox = new QCheckBox("自动解析");
    autoExtractCheckBox->setToolTip("自动抓取网页HTML，从中提取视频链接 (m3u8/mp4等)");
    autoExtractCheckBox->setChecked(true);
    urlLayout->addWidget(autoExtractCheckBox);

    analyzeBtn = new QPushButton("🔍 解析视频");
    urlLayout->addWidget(analyzeBtn);
    mainLayout->addLayout(urlLayout);

    // 第二行：画质选择、文件名、下载按钮、停止按钮
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

    stopBtn = new QPushButton("⏹ 停止");
    stopBtn->setEnabled(false);
    stopBtn->setStyleSheet("QPushButton:enabled { color: red; }");
    downloadLayout->addWidget(stopBtn);
    mainLayout->addLayout(downloadLayout);

    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressLayout->addWidget(progressBar, 1);

    statusLabel = new QLabel("");
    statusLabel->setMinimumWidth(280);
    statusLabel->setStyleSheet("color: #888; font-size: 12px;");
    progressLayout->addWidget(statusLabel);
    mainLayout->addLayout(progressLayout);

    logConsole = new QTextEdit();
    logConsole->setReadOnly(true);
    logConsole->setStyleSheet(
        "background-color: #1e1e1e; color: #d4d4d4; font-family: 'Menlo';");
    mainLayout->addWidget(logConsole);

    setCentralWidget(centralWidget);

    // 创建 QProcess 实例
    analyzeProcess = new QProcess(this);
    downloadProcess = new QProcess(this);
    aria2cProcess = new QProcess(this);

    // 信号连接：按钮
    connect(analyzeBtn, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);
    connect(downloadBtn, &QPushButton::clicked, this, &MainWindow::onDownloadClicked);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);

    // 信号连接：yt-dlp 解析进程
    connect(analyzeProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleAnalyzeFinished);
    connect(analyzeProcess, &QProcess::errorOccurred,
            this, &MainWindow::handleProcessError);

    // 信号连接：yt-dlp 下载进程
    connect(downloadProcess, &QProcess::readyReadStandardOutput,
            this, &MainWindow::handleDownloadOutput);
    connect(downloadProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleDownloadFinished);
    connect(downloadProcess, &QProcess::errorOccurred,
            this, &MainWindow::handleProcessError);

    // 信号连接：aria2c 进程
    connect(aria2cProcess, &QProcess::readyReadStandardOutput,
            this, &MainWindow::handleAria2cOutput);
    connect(aria2cProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleAria2cFinished);
    connect(aria2cProcess, &QProcess::errorOccurred,
            this, &MainWindow::handleProcessError);

    // 初始化工具管理器
    setupToolManager();

    // 初始化 URL 提取器
    urlExtractor = new UrlExtractor(this);
    connect(urlExtractor, &UrlExtractor::urlFound,
            this, &MainWindow::onUrlExtracted);
    connect(urlExtractor, &UrlExtractor::noUrlFound,
            this, &MainWindow::onUrlExtractFailed);
    connect(urlExtractor, &UrlExtractor::errorOccurred,
            this, &MainWindow::onUrlExtractError);
}

MainWindow::~MainWindow() {}

void MainWindow::setupToolManager() {
    toolManager = new ToolManager(this);

    connect(toolManager, &ToolManager::logMessage,
            this, &MainWindow::logMessage);
    connect(toolManager, &ToolManager::toolsReady, this, [this]() {
        ytdlpPath = toolManager->getYtdlpPath();
        ffmpegPath = toolManager->getFfmpegPath();
        aria2cPath = toolManager->getAria2cPath();
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

    if (PlatformUtils::isWindows()) {
        analyzeBtn->setEnabled(false);
        toolManager->checkAndDownloadTools();
    }
}

void MainWindow::logMessage(const QString &msg) {
    logConsole->append(msg);
}

// ============================================================
// URL 类型判断
// ============================================================
bool MainWindow::isMagnetOrTorrent(const QString &url) const {
    return url.startsWith("magnet:", Qt::CaseInsensitive)
        || url.endsWith(".torrent", Qt::CaseInsensitive);
}

// ============================================================
// UI 状态控制
// ============================================================
void MainWindow::setDownloadUIEnabled(bool enabled) {
    downloadBtn->setEnabled(enabled);
    analyzeBtn->setEnabled(enabled);
    stopBtn->setEnabled(!enabled);
    if (enabled) {
        statusLabel->clear();
        setWindowTitle("全能网页视频下载器 (支持 Cookie 绕过)");
    }
}

// ============================================================
// 解析按钮
// ============================================================
void MainWindow::onAnalyzeClicked() {
    currentUrl = urlInput->text().trimmed();
    if (currentUrl.isEmpty()) {
        logMessage("❌ 请先输入链接");
        return;
    }

    analyzeBtn->setEnabled(false);
    downloadBtn->setEnabled(false);
    resolutionBox->clear();
    formatMap.clear();

    // magnet / torrent 链接无需解析，直接启用下载按钮
    if (isMagnetOrTorrent(currentUrl)) {
        logMessage("🔗 检测到磁力/种子链接，可直接下载");
        formatMap.insert("BT 直接下载", "bt");
        resolutionBox->addItem("BT 直接下载");
        nameInput->setText("BT下载任务");
        nameInput->setEnabled(false);
        downloadBtn->setEnabled(true);
        analyzeBtn->setEnabled(true);
        return;
    }

    // 自动解析：先抓取网页 HTML 提取视频 URL
    if (autoExtractCheckBox->isChecked()) {
        logMessage("🔍 正在自动解析网页，提取视频链接...");
        urlExtractor->extract(currentUrl, cookieBox->currentText());
        return;
    }

    // 直接使用 yt-dlp 解析
    logMessage("⏳ 正在尝试解析网页数据 (包含 Cookie 注入)...");
    startYtdlpAnalyze();
}

void MainWindow::onUrlExtracted(const QString &extractedUrl) {
    logMessage("✅ 自动解析成功: " + extractedUrl);
    currentUrl = extractedUrl;
    logMessage("⏳ 正在使用 yt-dlp 获取视频详情...");
    startYtdlpAnalyze();
}

void MainWindow::onUrlExtractFailed(const QString &pageUrl) {
    Q_UNUSED(pageUrl)
    logMessage("⚠️ 未从网页中提取到视频链接，尝试使用 yt-dlp 直接解析...");
    logMessage("⏳ 正在尝试解析网页数据 (包含 Cookie 注入)...");
    startYtdlpAnalyze();
}

void MainWindow::onUrlExtractError(const QString &errorMsg) {
    logMessage("⚠️ " + errorMsg + "，尝试使用 yt-dlp 直接解析...");
    logMessage("⏳ 正在尝试解析网页数据 (包含 Cookie 注入)...");
    startYtdlpAnalyze();
}

void MainWindow::startYtdlpAnalyze() {
    QStringList args;
    QString browser = cookieBox->currentText();
    if (browser != "无") {
        args << "--cookies-from-browser" << browser;
    }

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

// ============================================================
// 下载按钮：根据 URL 类型路由到 aria2c 或 yt-dlp
// ============================================================
void MainWindow::onDownloadClicked() {
    retryCount = 0;
    currentUrl = urlInput->text().trimmed();

    if (isMagnetOrTorrent(currentUrl)) {
        startAria2cDownload(currentUrl);
    } else {
        startYtdlpDownload();
    }
}

// ============================================================
// yt-dlp 下载（支持断点续传）
// ============================================================
void MainWindow::startYtdlpDownload() {
    try {
        QString formatId = formatMap[resolutionBox->currentText()];
        QString outName = nameInput->text();

        if (outName.isEmpty()) {
            logMessage("❌ 请输入文件名");
            return;
        }

        QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        downloadProcess->setWorkingDirectory(downloadDir);

        logMessage("🚀 开始下载: " + outName);
        logMessage("📁 保存至: " + downloadDir);
        usingAria2c = false;
        setDownloadUIEnabled(false);
        progressBar->setValue(0);
        statusLabel->setText("正在连接...");

        QStringList args;
        QString browser = cookieBox->currentText();
        if (browser != "无") {
            args << "--cookies-from-browser" << browser;
        }

        if (!ffmpegPath.isEmpty() && QFile::exists(ffmpegPath)) {
            args << "--ffmpeg-location" << ffmpegPath;
        }

        // 断点续传
        args << "--continue";
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
        setDownloadUIEnabled(true);
    } catch (...) {
        logMessage("❌ 启动下载时发生未知错误");
        setDownloadUIEnabled(true);
    }
}

// ============================================================
// 进度详情更新（通用）
// ============================================================
void MainWindow::updateProgressDetail(int percent, const QString &size,
                                       const QString &speed, const QString &eta) {
    progressBar->setValue(percent);

    // 构建状态文本: "42.3%  │  10.00MiB / 25.00MiB  │  2.50MiB/s  │  剩余 00:03"
    QString detail = QString("%1%").arg(percent);
    if (!size.isEmpty())
        detail += "  │  " + size;
    if (!speed.isEmpty())
        detail += "  │  " + speed;
    if (!eta.isEmpty() && eta != "Unknown")
        detail += "  │  剩余 " + eta;

    statusLabel->setText(detail);

    // 窗口标题同步显示进度
    setWindowTitle(QString("%1% - 全能网页视频下载器").arg(percent));
}

// ============================================================
// aria2c 下载（磁力/BT，支持断点续传）
// ============================================================
void MainWindow::startAria2cDownload(const QString &url) {
    if (aria2cPath.isEmpty()) {
        logMessage("❌ 未找到 aria2c，请先安装 aria2c（macOS: brew install aria2）");
        return;
    }

    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    aria2cProcess->setWorkingDirectory(downloadDir);

    logMessage("🧲 启动 BT/磁力下载");
    logMessage("📁 保存至: " + downloadDir);
    logMessage("⏳ 正在连接 DHT 网络，请耐心等待...");
    usingAria2c = true;
    setDownloadUIEnabled(false);
    progressBar->setValue(0);
    statusLabel->setText("正在连接 DHT 网络...");

    QStringList args;
    args << "--continue=true";
    args << "--max-connection-per-server=16";
    args << "--split=16";
    args << "--min-split-size=1M";
    args << "--enable-dht=true";
    args << "--enable-peer-exchange=true";
    args << "--seed-time=0";
    args << "--auto-file-allocation=none";
    args << "--console-log-level=notice";
    args << "--summary-interval=1";
    args << "--dir" << downloadDir;
    args << url;

    aria2cProcess->start(aria2cPath, args);
}

// ============================================================
// aria2c 下载输出解析（详细进度）
//
// aria2c 输出格式示例：
//   [#abc123 1.2MiB/3.4GiB(0%) CN:16 DL:2.5MiB ETA:30m]
//   [#abc123 3.4GiB/3.4GiB(99%) CN:4 DL:1.2MiB ETA:5s]
// ============================================================
void MainWindow::handleAria2cOutput() {
    try {
        QString output = QString::fromUtf8(aria2cProcess->readAllStandardOutput());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);

        for (const QString &line : lines) {
            // 匹配百分比
            QRegularExpression pctRe("\\(([0-9.]+)%\\)");
            QRegularExpressionMatch pctMatch = pctRe.match(line);
            if (!pctMatch.hasMatch())
                continue;

            int percent = static_cast<int>(pctMatch.captured(1).toDouble());

            // 匹配已下载/总大小 (如 "1.2MiB/3.4GiB")
            QRegularExpression sizeRe("([0-9.]+\\s*[KMG]?i?B)/([0-9.]+\\s*[KMG]?i?B)");
            QRegularExpressionMatch sizeMatch = sizeRe.match(line);
            QString sizeStr;
            if (sizeMatch.hasMatch()) {
                sizeStr = sizeMatch.captured(1) + " / " + sizeMatch.captured(2);
            }

            // 匹配下载速度 (如 "DL:2.5MiB")
            QRegularExpression speedRe("DL:([0-9.]+\\s*[KMG]?i?B)");
            QRegularExpressionMatch speedMatch = speedRe.match(line);
            QString speedStr;
            if (speedMatch.hasMatch()) {
                speedStr = speedMatch.captured(1) + "/s";
            }

            // 匹配 ETA (如 "ETA:30m" 或 "ETA:5s")
            QRegularExpression etaRe("ETA:([0-9hms]+)");
            QRegularExpressionMatch etaMatch = etaRe.match(line);
            QString etaStr;
            if (etaMatch.hasMatch()) {
                etaStr = etaMatch.captured(1);
            }

            // 匹配连接数 (如 "CN:16")
            QRegularExpression cnRe("CN:(\\d+)");
            QRegularExpressionMatch cnMatch = cnRe.match(line);
            if (cnMatch.hasMatch()) {
                sizeStr += "  │  连接: " + cnMatch.captured(1);
            }

            updateProgressDetail(percent, sizeStr, speedStr, etaStr);
        }
    } catch (...) {
        // 忽略输出解析错误
    }
}

void MainWindow::handleAria2cFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus)

    if (exitCode == 0) {
        updateProgressDetail(100, "", "", "已完成");
        logMessage("🎉 BT/磁力下载完成！");
        retryCount = 0;
    } else {
        if (retryCount < MAX_RETRIES) {
            retryCount++;
            logMessage(QString("⚠️ 下载中断，%1 秒后进行第 %2/%3 次重试...")
                           .arg(3).arg(retryCount).arg(MAX_RETRIES));
            statusLabel->setText(QString("网络中断，%1 秒后重试...").arg(3));
            QTimer::singleShot(3000, this, &MainWindow::retryDownload);
            return;
        }
        logMessage("❌ BT/磁力下载失败，已达最大重试次数。");
    }

    setDownloadUIEnabled(true);
}

// ============================================================
// yt-dlp 下载输出解析（详细进度）
//
// yt-dlp --newline 输出格式示例：
//   [download]   0.0% of    1.50MiB at  Unknown B/s ETA Unknown
//   [download]  42.3% of    1.50MiB at    2.50MiB/s ETA 00:03
//   [download] 100% of    1.50MiB in 00:04
// ============================================================
void MainWindow::handleDownloadOutput() {
    try {
        QString output = QString::fromUtf8(downloadProcess->readAllStandardOutput());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);

        for (const QString &line : lines) {
            if (!line.contains("[download]"))
                continue;

            // 匹配百分比
            QRegularExpression pctRe("\\[download\\]\\s+([0-9.]+)%");
            QRegularExpressionMatch pctMatch = pctRe.match(line);
            if (!pctMatch.hasMatch())
                continue;

            int percent = static_cast<int>(pctMatch.captured(1).toDouble());

            // 匹配已下载 / 总大小  (如 "of   10.00MiB")
            QRegularExpression sizeRe("of\\s+([0-9.]+\\s*[KMG]?iB)");
            QRegularExpressionMatch sizeMatch = sizeRe.match(line);
            QString sizeStr;
            if (sizeMatch.hasMatch()) {
                sizeStr = sizeMatch.captured(1).trimmed();
            }

            // 匹配速度 (如 "at  2.50MiB/s")
            QRegularExpression speedRe("at\\s+([0-9.]+\\s*[KMG]?iB/s)");
            QRegularExpressionMatch speedMatch = speedRe.match(line);
            QString speedStr;
            if (speedMatch.hasMatch()) {
                speedStr = speedMatch.captured(1).trimmed();
            }

            // 匹配 ETA (如 "ETA 00:03")
            QRegularExpression etaRe("ETA\\s+([0-9:]+|Unknown)");
            QRegularExpressionMatch etaMatch = etaRe.match(line);
            QString etaStr;
            if (etaMatch.hasMatch()) {
                etaStr = etaMatch.captured(1);
            }

            // 完成状态：匹配 "in 00:04"
            QRegularExpression doneRe("in\\s+([0-9:]+)$");
            QRegularExpressionMatch doneMatch = doneRe.match(line);
            if (doneMatch.hasMatch()) {
                etaStr = "已完成";
                speedStr = "";
            }

            updateProgressDetail(percent, sizeStr, speedStr, etaStr);
        }
    } catch (...) {
        // 忽略输出解析错误
    }
}

void MainWindow::handleDownloadFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus)

    if (exitCode == 0) {
        updateProgressDetail(100, "", "", "已完成");
        logMessage("🎉 下载任务圆满完成！");
        retryCount = 0;
    } else {
        if (retryCount < MAX_RETRIES) {
            retryCount++;
            logMessage(QString("⚠️ 下载中断，%1 秒后进行第 %2/%3 次重试（支持断点续传）...")
                           .arg(3).arg(retryCount).arg(MAX_RETRIES));
            statusLabel->setText(QString("网络中断，%1 秒后重试...").arg(3));
            QTimer::singleShot(3000, this, &MainWindow::retryDownload);
            return;
        }
        logMessage("❌ 下载出错，请检查网络或是否需要更新 yt-dlp。");
    }

    setDownloadUIEnabled(true);
}

// ============================================================
// 断网重试
// ============================================================
void MainWindow::retryDownload() {
    logMessage(QString("🔄 正在进行第 %1 次重试...").arg(retryCount));
    setDownloadUIEnabled(false);
    progressBar->setValue(0);

    if (usingAria2c) {
        startAria2cDownload(currentUrl);
    } else {
        startYtdlpDownload();
    }
}

// ============================================================
// 停止按钮
// ============================================================
void MainWindow::onStopClicked() {
    if (usingAria2c && aria2cProcess->state() != QProcess::NotRunning) {
        aria2cProcess->kill();
        logMessage("⏹ BT/磁力下载已停止");
    } else if (downloadProcess->state() != QProcess::NotRunning) {
        downloadProcess->kill();
        logMessage("⏹ 下载已停止");
    }
    setDownloadUIEnabled(true);
}

// ============================================================
// 进程启动错误
// ============================================================
void MainWindow::handleProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error)
    logMessage("❌ 引擎启动失败，请确保系统已安装 yt-dlp、ffmpeg 和 aria2c。");
    setDownloadUIEnabled(true);
}
