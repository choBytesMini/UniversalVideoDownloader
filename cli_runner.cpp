#include "cli_runner.h"
#include "download_utils.h"
#include "platformutils.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <cstdio>

// ANSI 颜色码
const char *CliRunner::RESET = "\033[0m";
const char *CliRunner::BOLD  = "\033[1m";
const char *CliRunner::GREEN = "\033[32m";
const char *CliRunner::YELLOW = "\033[33m";
const char *CliRunner::RED   = "\033[31m";
const char *CliRunner::CYAN  = "\033[36m";
const char *CliRunner::DIM   = "\033[2m";

CliRunner::CliRunner(QObject *parent) : QObject(parent), process(nullptr) {}

void CliRunner::printHeader() {
    printf("\n");
    printf("%s╔══════════════════════════════════════════════╗%s\n", CYAN, RESET);
    printf("%s║  全能网页视频下载器  v2.0  (终端模式)        ║%s\n", CYAN, RESET);
    printf("%s╚══════════════════════════════════════════════╝%s\n", CYAN, RESET);
    printf("\n");
}

void CliRunner::printUsage() {
    printHeader();
    printf("%s用法:%s\n", BOLD, RESET);
    printf("  UniversalVideoDownloader %s-c%s <url> [选项]\n", GREEN, RESET);
    printf("  UniversalVideoDownloader %s--cli%s <url> [选项]\n\n", GREEN, RESET);

    printf("%s选项:%s\n", BOLD, RESET);
    printf("  %s-c, --cli%s         终端模式（无 GUI）\n", GREEN, RESET);
    printf("  %s-o, --output%s <dir>  下载目录（默认: ~/Downloads）\n", GREEN, RESET);
    printf("  %s-C, --cookie%s <br>   浏览器 Cookie（firefox/chrome/edge/safari）\n", GREEN, RESET);
    printf("  %s-p, --playlist%s      合集模式\n", GREEN, RESET);
    printf("  %s-f, --format%s <fmt>   指定画质格式（默认: bestvideo+bestaudio/best）\n", GREEN, RESET);
    printf("  %s-v, --verbose%s        显示详细日志\n", GREEN, RESET);
    printf("  %s-h, --help%s           显示帮助\n\n", GREEN, RESET);

    printf("%s示例:%s\n", BOLD, RESET);
    printf("  %s# 下载视频%s\n", DIM, RESET);
    printf("  UniversalVideoDownloader -c https://www.bilibili.com/video/BV1xx411c7XX\n");
    printf("  %s# 磁力下载%s\n", DIM, RESET);
    printf("  UniversalVideoDownloader -c \"magnet:?xt=urn:btih:...\"\n");
    printf("  %s# 使用 Cookie 下载 B站大会员视频%s\n", DIM, RESET);
    printf("  UniversalVideoDownloader -c <url> -C firefox\n");
    printf("  %s# 指定下载目录%s\n", DIM, RESET);
    printf("  UniversalVideoDownloader -c <url> -o ~/Videos\n\n");
    fflush(stdout);
}

void CliRunner::printProgress(const QString &line) {
    // 用 \r 覆盖当前行
    printf("\r\033[K%s", line.toUtf8().constData());
    fflush(stdout);
}

int CliRunner::run(int argc, char *argv[]) {
    QStringList args;
    for (int i = 1; i < argc; ++i) {
        args << QString::fromLocal8Bit(argv[i]);
    }

    // 先检查 --help / -h（在移除 --cli 之前）
    for (const QString &a : args) {
        if (a == "--help" || a == "-h") {
            printUsage();
            return 0;
        }
    }

    // 检查 --cli / -c
    bool cliMode = false;
    for (int i = 0; i < args.size(); ++i) {
        if (args[i] == "--cli" || args[i] == "-c") {
            cliMode = true;
            args.removeAt(i);
            break;
        }
    }

    if (!cliMode) {
        return -1; // 不是 CLI 模式，交给 GUI
    }

    // 解析参数
    currentUrl.clear();
    outputDir = DownloadUtils::defaultDownloadDir();
    cookieBrowser.clear();
    isMagnet = false;
    verbose = false;
    QString formatSpec = "bestvideo+bestaudio/best";
    bool playlistMode = false;

    for (int i = 0; i < args.size(); ++i) {
        const QString &a = args[i];
        if (a == "-o" || a == "--output") {
            if (i + 1 < args.size()) outputDir = args[++i];
        } else if (a == "-C" || a == "--cookie") {
            if (i + 1 < args.size()) cookieBrowser = args[++i];
        } else if (a == "-p" || a == "--playlist") {
            playlistMode = true;
        } else if (a == "-f" || a == "--format") {
            if (i + 1 < args.size()) formatSpec = args[++i];
        } else if (a == "-v" || a == "--verbose") {
            verbose = true;
        } else if (!a.startsWith('-')) {
            currentUrl = a;
        }
    }

    if (currentUrl.isEmpty()) {
        printf("%s❌ 错误: 请提供下载链接%s\n", RED, RESET);
        printf("   使用 -h 查看帮助\n");
        return 1;
    }

    // 初始化路径
    ytdlpPath = PlatformUtils::findYtdlpPath();
    ffmpegPath = PlatformUtils::findFfmpegPath();
    aria2cPath = PlatformUtils::findAria2cPath();
    isMagnet = DownloadUtils::isMagnetOrTorrent(currentUrl);

    printHeader();

    // 显示配置信息
    printf("%s🔗 链接:%s %s\n", BOLD, RESET, currentUrl.toUtf8().constData());
    printf("%s📁 目录:%s %s\n", BOLD, RESET, outputDir.toUtf8().constData());
    if (!cookieBrowser.isEmpty())
        printf("%s🍪 Cookie:%s %s\n", BOLD, RESET, cookieBrowser.toUtf8().constData());
    if (isMagnet)
        printf("%s🧲 模式:%s BT/磁力下载 (aria2c)\n", BOLD, RESET);
    else
        printf("%s🎬 模式:%s 视频下载 (yt-dlp)\n", BOLD, RESET);
    printf("\n");

    // 确保下载目录存在
    QDir().mkpath(outputDir);

    // 创建进程
    process = new QProcess(this);
    process->setWorkingDirectory(outputDir);

    connect(process, &QProcess::readyReadStandardOutput,
            this, &CliRunner::onProcessReadyRead);
    connect(process, &QProcess::readyReadStandardError,
            this, &CliRunner::onProcessReadyRead);
    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CliRunner::onProcessFinished);
    connect(process, &QProcess::errorOccurred,
            this, &CliRunner::onProcessError);

    startDownload();
    return QCoreApplication::exec();
}

void CliRunner::startDownload() {
    if (isMagnet) {
        // aria2c 模式
        if (aria2cPath.isEmpty()) {
            printf("%s❌ 未找到 aria2c%s\n", RED, RESET);
            printf("   macOS: brew install aria2\n");
            printf("   Windows: 从 https://github.com/aria2/aria2/releases 下载\n");
            QCoreApplication::exit(1);
            return;
        }

        printf("%s⏳ 正在连接 DHT 网络...%s\n", YELLOW, RESET);

        QStringList args = DownloadUtils::buildAria2cArgs(outputDir, currentUrl);
        process->start(aria2cPath, args);
    } else {
        // yt-dlp 模式 — 先解析再下载
        printf("%s⏳ 正在解析视频信息...%s\n", YELLOW, RESET);

        // 第一步：用 -J 获取视频信息
        QProcess *analyzeProc = new QProcess(this);
        QStringList analyzeArgs;
        analyzeArgs.append(DownloadUtils::buildCookieArgs(cookieBrowser));
        analyzeArgs << "--no-playlist" << "-J" << currentUrl;

        connect(analyzeProc,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, analyzeProc](int exitCode, QProcess::ExitStatus) {
            if (exitCode != 0) {
                printf("%s❌ 解析失败%s\n", RED, RESET);
                if (verbose) {
                    QString err = QString::fromUtf8(analyzeProc->readAllStandardError());
                    fprintf(stderr, "%s\n", err.toUtf8().constData());
                }
                analyzeProc->deleteLater();
                QCoreApplication::exit(1);
                return;
            }

            QByteArray jsonData = analyzeProc->readAllStandardOutput();
            analyzeProc->deleteLater();

            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            QJsonObject root = doc.object();
            QString title = root["title"].toString();

            printf("%s✅ 标题:%s %s\n", GREEN, RESET, title.toUtf8().constData());

            // 显示可用画质
            QJsonArray formats = root["formats"].toArray();
            int count = 0;
            for (const QJsonValue &val : formats) {
                QJsonObject fmt = val.toObject();
                if (fmt["vcodec"].toString() == "none") continue;
                int h = fmt["height"].toInt(0);
                if (h > 0) {
                    QString ext = fmt["ext"].toString();
                    QString fid = fmt["format_id"].toString();
                    printf("   %s%dp%s (%s) [%s]\n",
                           CYAN, h, RESET, ext.toUtf8().constData(),
                           fid.toUtf8().constData());
                    count++;
                }
            }
            if (count > 0)
                printf("   共 %d 个画质可选，使用 -f <format_id> 指定\n", count);
            printf("\n");

            // 开始下载
            QString outName = title + ".mp4";
            printf("%s🚀 开始下载:%s %s\n", GREEN, RESET, outName.toUtf8().constData());

            QStringList dlArgs;
            dlArgs.append(DownloadUtils::buildCookieArgs(cookieBrowser));
            if (!ffmpegPath.isEmpty() && QFile::exists(ffmpegPath)) {
                dlArgs << "--ffmpeg-location" << ffmpegPath;
            }
            dlArgs << "--continue"
                   << "-f" << "bestvideo+bestaudio/best"
                   << "--merge-output-format" << "mp4"
                   << "--newline"
                   << "--no-playlist"
                   << "-o" << outName
                   << currentUrl;

            process->start(ytdlpPath, dlArgs);
        });

        analyzeProc->start(ytdlpPath, analyzeArgs);
    }
}

void CliRunner::onProcessReadyRead() {
    // 读取 stdout 和 stderr
    QByteArray stdoutData = process->readAllStandardOutput();
    QByteArray stderrData = process->readAllStandardError();

    QString output = QString::fromUtf8(stdoutData);
    QString errors = QString::fromUtf8(stderrData);

    // yt-dlp 进度行以 [download] 开头
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        DownloadProgress p = DownloadProgressParser::parseYtdlpLine(line);
        if (p.valid) {
            QString progressLine = QString("%1⬇ %2%%%3")
                .arg(GREEN).arg(p.percent).arg(RESET);
            if (!p.size.isEmpty())
                progressLine += QString("  %1").arg(p.size);
            if (!p.speed.isEmpty())
                progressLine += QString("  %1%2/s%3").arg(CYAN, p.speed, RESET);
            if (!p.eta.isEmpty() && p.eta != "Unknown")
                progressLine += QString("  ETA %1").arg(p.eta);

            printProgress(progressLine);
        } else if (line.contains("[Merger]") || line.contains("[ExtractAudio]")) {
            // 合并/提取音频
            printf("\r\033[K%s⏳ %s%s\n", YELLOW, line.trimmed().toUtf8().constData(), RESET);
        } else if (verbose && !line.trimmed().isEmpty()) {
            printf("\r\033[K%s\n", line.toUtf8().constData());
        }
    }

    // aria2c 进度包含 (XX%)
    if (output.contains("(%)") || output.contains("%)")) {
        DownloadProgress p = DownloadProgressParser::parseAria2cLine(output);
        if (p.valid) {
            QString progressLine = QString("%1🧲 %2%%%3")
                .arg(GREEN).arg(p.percent).arg(RESET);
            if (!p.size.isEmpty())
                progressLine += QString("  %1").arg(p.size);
            if (!p.speed.isEmpty())
                progressLine += QString("  %1%2%3").arg(CYAN, p.speed, RESET);
            if (!p.eta.isEmpty())
                progressLine += QString("  ETA %1").arg(p.eta);

            printProgress(progressLine);
        }
    }

    // 输出错误信息
    if (!errors.isEmpty() && verbose) {
        printf("\r\033[K%s%s%s\n", DIM, errors.trimmed().toUtf8().constData(), RESET);
    }
}

void CliRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus)

    printf("\n"); // 换行，结束进度覆盖

    if (exitCode == 0) {
        printf("%s✅ 下载完成！%s\n", GREEN, RESET);
        printf("%s📁 保存至: %s%s\n", BOLD, outputDir.toUtf8().constData(), RESET);
    } else {
        printf("%s❌ 下载失败 (exit code: %d)%s\n", RED, exitCode, RESET);
    }

    QCoreApplication::exit(exitCode);
}

void CliRunner::onProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error)
    printf("\n%s❌ 进程启动失败%s\n", RED, RESET);
    if (isMagnet)
        printf("   请确保已安装 aria2c\n");
    else
        printf("   请确保已安装 yt-dlp 和 ffmpeg\n");
    QCoreApplication::exit(1);
}
