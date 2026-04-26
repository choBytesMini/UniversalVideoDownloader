#include "mainwindow.h"
#include "download_utils.h"
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
#include <QTimer>
#include <QUrl>
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

    // 第二行：视频选择（如果有多视频/合集）、画质选择、文件名、下载按钮、停止按钮
    QHBoxLayout *downloadLayout = new QHBoxLayout();
    
    videoLabel = new QLabel("选择视频:");
    videoBox = new QComboBox();
    videoBox->setMinimumWidth(150);
    downloadLayout->addWidget(videoLabel);
    downloadLayout->addWidget(videoBox);
    videoLabel->hide();
    videoBox->hide();

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

    connect(videoBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onVideoSelectionChanged);

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
    connect(aria2cProcess, &QProcess::readyReadStandardError,
            this, &MainWindow::handleAria2cError);
    connect(aria2cProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::handleAria2cFinished);
    connect(aria2cProcess, &QProcess::errorOccurred,
            this, &MainWindow::handleProcessError);

    // 初始化工具管理器
    setupToolManager();

    // 初始化 URL 提取器
    urlExtractor = new UrlExtractor(this);
    connect(urlExtractor, &UrlExtractor::urlsFound,
            this, &MainWindow::onUrlsExtracted);
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
    if (currentUrl.startsWith('"') && currentUrl.endsWith('"')) {
        currentUrl = currentUrl.mid(1, currentUrl.length() - 2);
    }
    
    if (currentUrl.startsWith("file://", Qt::CaseInsensitive)) {
#ifdef Q_OS_WIN
        if (currentUrl.startsWith("file:///", Qt::CaseInsensitive)) {
            currentUrl = currentUrl.mid(8);
        } else {
            currentUrl = currentUrl.mid(7);
        }
#else
        currentUrl = currentUrl.mid(7);
#endif
        // URL 解码：file:// URL 中的特殊字符可能被百分号编码
        currentUrl = QUrl::fromPercentEncoding(currentUrl.toUtf8());
    }

    QRegularExpression infoHashRegex("^[0-9a-fA-F]{40}$");
    if (infoHashRegex.match(currentUrl).hasMatch()) {
        currentUrl = "magnet:?xt=urn:btih:" + currentUrl;
    }

    if (currentUrl.isEmpty()) {
        logMessage("❌ 请先输入链接");
        return;
    }

    analyzeBtn->setEnabled(false);
    downloadBtn->setEnabled(false);
    resolutionBox->clear();
    formatMap.clear();
    videoBox->clear();
    videoMap.clear();
    videoLabel->hide();
    videoBox->hide();

    // magnet / torrent 链接无需解析，直接启用下载按钮
    if (DownloadUtils::isMagnetOrTorrent(currentUrl)) {
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

void MainWindow::onUrlsExtracted(const QStringList &extractedUrls) {
    if (extractedUrls.isEmpty()) return;
    
    if (extractedUrls.size() == 1) {
        logMessage("✅ 自动解析成功: " + extractedUrls.first());
        currentUrl = extractedUrls.first();
    } else {
        logMessage(QString("✅ 自动解析发现 %1 个视频，已默认选中第一个。").arg(extractedUrls.size()));
        
        // 阻塞信号，避免触发 onVideoSelectionChanged
        bool oldState = videoBox->blockSignals(true);
        videoBox->clear();
        videoMap.clear();
        
        for (int i = 0; i < extractedUrls.size(); ++i) {
            QString name = QString("提取视频 %1").arg(i + 1);
            videoBox->addItem(name);
            videoMap.insert(name, extractedUrls.at(i));
        }
        
        videoBox->setCurrentIndex(0);
        videoBox->blockSignals(oldState);
        
        videoLabel->show();
        videoBox->show();
        
        currentUrl = extractedUrls.first();
    }
    
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

void MainWindow::onVideoSelectionChanged(int index) {
    if (index < 0 || videoBox->itemText(index).isEmpty()) return;
    
    QString selectedName = videoBox->itemText(index);
    if (!videoMap.contains(selectedName)) return;

    if (selectedName == "全集合并下载/自动编号") {
        currentUrl = videoMap[selectedName];
        playlistCheckBox->setChecked(true);
        resolutionBox->clear();
        formatMap.clear();
        formatMap.insert("自动最佳合集画质", "bestvideo+bestaudio/best");
        resolutionBox->addItem("自动最佳合集画质");
        nameInput->setText("合集自动编号下载");
        nameInput->setEnabled(false);
        downloadBtn->setEnabled(true);
        return;
    }

    currentUrl = videoMap[selectedName];
    logMessage(QString("👉 已选择视频: %1").arg(selectedName));
    
    // 取消合集模式，重新解析该单集
    playlistCheckBox->setChecked(false);
    
    analyzeBtn->setEnabled(false);
    downloadBtn->setEnabled(false);
    resolutionBox->clear();
    formatMap.clear();
    
    logMessage("⏳ 正在获取选中视频的详细信息...");
    startYtdlpAnalyze();
}

void MainWindow::startYtdlpAnalyze() {
    QStringList args;
    args.append(DownloadUtils::buildCookieArgs(cookieBox->currentText()));

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

            if (root.contains("entries")) {
                bool oldState = videoBox->blockSignals(true);
                videoBox->clear();
                videoMap.clear();

                videoBox->addItem("全集合并下载/自动编号");
                videoMap.insert("全集合并下载/自动编号", currentUrl);

                QJsonArray entries = root["entries"].toArray();
                for (int i = 0; i < entries.size(); ++i) {
                    QJsonObject entry = entries[i].toObject();
                    QString entryTitle = entry["title"].toString();
                    if (entryTitle.isEmpty()) entryTitle = QString("视频 %1").arg(i + 1);
                    QString entryUrl = entry["url"].toString();
                    if (entryUrl.isEmpty()) entryUrl = entry["id"].toString();

                    videoBox->addItem(entryTitle);
                    videoMap.insert(entryTitle, entryUrl);
                }

                videoBox->setCurrentIndex(0);
                videoBox->blockSignals(oldState);

                videoLabel->show();
                videoBox->show();

                logMessage("✨ 你可以在 '选择视频' 中选择具体哪一P下载，或者选择合并下载。");
            }

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

    if (DownloadUtils::isMagnetOrTorrent(currentUrl)) {
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

        QString downloadDir = DownloadUtils::defaultDownloadDir();
        downloadProcess->setWorkingDirectory(downloadDir);

        logMessage("🚀 开始下载: " + outName);
        logMessage("📁 保存至: " + downloadDir);
        usingAria2c = false;
        setDownloadUIEnabled(false);
        progressBar->setValue(0);
        statusLabel->setText("正在连接...");

        QStringList args;
        args.append(DownloadUtils::buildCookieArgs(cookieBox->currentText()));

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

    // 本地种子文件：检查文件是否存在
    if (!url.startsWith("magnet:", Qt::CaseInsensitive) && url.endsWith(".torrent", Qt::CaseInsensitive)) {
        if (!QFile::exists(url)) {
            logMessage("❌ 种子文件不存在: " + url);
            setDownloadUIEnabled(true);
            return;
        }
        logMessage("📂 种子文件: " + url);
    }

    QString downloadDir = DownloadUtils::defaultDownloadDir();
    aria2cProcess->setWorkingDirectory(downloadDir);

    logMessage("🧲 启动 BT/磁力下载");
    logMessage("📁 保存至: " + downloadDir);
    logMessage("⏳ 正在连接 DHT 网络，请耐心等待...");
    usingAria2c = true;
    setDownloadUIEnabled(false);
    progressBar->setValue(0);
    statusLabel->setText("正在连接 DHT 网络...");

    QStringList args = DownloadUtils::buildAria2cArgs(downloadDir, url);
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
            DownloadProgress p = DownloadProgressParser::parseAria2cLine(line);
            if (p.valid) {
                QString sizeStr = p.size;
                if (p.connections > 0)
                    sizeStr += "  │  连接: " + QString::number(p.connections);

                updateProgressDetail(p.percent, sizeStr, p.speed, p.eta);
            } else {
                // 展示 aria2c 的非进度输出（连接状态、tracker 响应、错误等）
                logMessage(line.trimmed());
            }
        }
    } catch (...) {
        // 忽略输出解析错误
    }
}

void MainWindow::handleAria2cError() {
    QString err = QString::fromUtf8(aria2cProcess->readAllStandardError());
    QStringList lines = err.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        logMessage("[aria2c] " + line.trimmed());
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
        logMessage("💡 可能原因：");
        logMessage("   1. 当前种子无可用做种者（seeders=0）");
        logMessage("   2. Tracker 服务器不可达");
        logMessage("   3. DHT 网络未找到 peers");
        logMessage("   4. 防火墙阻挡了 BT 端口");
        logMessage("   请确认种子有效或稍后重试。");
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
            DownloadProgress p = DownloadProgressParser::parseYtdlpLine(line);
            if (!p.valid)
                continue;

            if (p.eta == QString::fromUtf8("已完成"))
                p.speed.clear();

            updateProgressDetail(p.percent, p.size, p.speed, p.eta);
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
        QString errorOutput = QString::fromUtf8(downloadProcess->readAllStandardError());
        if (!errorOutput.isEmpty()) {
            logMessage("❌ yt-dlp 错误输出: " + errorOutput.trimmed());
        }
        
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
    const char* errorMsgs[] = {
        "进程启动失败：可执行文件不存在或权限不足",
        "进程启动超时",
        "进程写入错误",
        "进程读取错误",
        "未知错误"
    };
    int idx = static_cast<int>(error);
    if (idx >= 0 && idx < 5) {
        logMessage(QString("❌ %1").arg(errorMsgs[idx]));
    } else {
        logMessage("❌ 引擎启动失败，请确保系统已安装 yt-dlp、ffmpeg 和 aria2c。");
    }
    setDownloadUIEnabled(true);
}
