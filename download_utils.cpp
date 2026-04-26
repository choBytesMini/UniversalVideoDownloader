#include "download_utils.h"
#include <QRegularExpression>
#include <QStandardPaths>

namespace DownloadUtils {

bool isMagnetOrTorrent(const QString &url) {
    QString cleanUrl = url.trimmed();
    if (cleanUrl.startsWith('"') && cleanUrl.endsWith('"')) {
        cleanUrl = cleanUrl.mid(1, cleanUrl.length() - 2);
    }

    // 40-character hex info hash
    QRegularExpression infoHashRegex("^[0-9a-fA-F]{40}$");
    if (infoHashRegex.match(cleanUrl).hasMatch()) {
        return true;
    }

    // magnet links
    if (cleanUrl.startsWith("magnet:", Qt::CaseInsensitive)) {
        return true;
    }

    // .torrent with optional url parameters or trailing quotes
    QRegularExpression torrentRegex("\\.torrent([\\?\"'].*)?$", QRegularExpression::CaseInsensitiveOption);
    if (torrentRegex.match(cleanUrl).hasMatch()) {
        return true;
    }

    return false;
}

QString defaultDownloadDir() {
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QStringList buildAria2cArgs(const QString &outputDir, const QString &url) {
    QStringList args = {
        "--continue=true",
        "--max-connection-per-server=16",
        "--split=16",
        "--min-split-size=1M",
        "--enable-dht=true",
        "--enable-peer-exchange=true",
        "--seed-time=0",
        "--bt-stop-timeout=300",
        "--connect-timeout=60",
        "--bt-tracker-connect-timeout=30",
        "--bt-tracker-timeout=60",
        "--console-log-level=notice",
        "--summary-interval=1",
        "--dir", outputDir,
    };

    // magnet 链接需要保存元数据以便断点续传
    if (url.startsWith("magnet:", Qt::CaseInsensitive)) {
        args << "--bt-save-metadata=true";
    }

    args << url;
    return args;
}

QStringList buildCookieArgs(const QString &browser) {
    if (browser.isEmpty() || browser == "无")
        return {};
    return {"--cookies-from-browser", browser};
}

} // namespace DownloadUtils

// ============================================================
// DownloadProgressParser 实现
// ============================================================

DownloadProgress DownloadProgressParser::parseYtdlpLine(const QString &line) {
    DownloadProgress p;

    if (!line.contains("[download]"))
        return p;

    // 匹配百分比
    QRegularExpression pctRe("\\[download\\]\\s+([0-9.]+)%");
    QRegularExpressionMatch pctMatch = pctRe.match(line);
    if (!pctMatch.hasMatch())
        return p;

    p.valid = true;
    p.percent = static_cast<int>(pctMatch.captured(1).toDouble());

    // 匹配总大小 (如 "of   10.00MiB")
    QRegularExpression sizeRe("of\\s+([0-9.]+\\s*[KMG]?iB)");
    QRegularExpressionMatch sizeMatch = sizeRe.match(line);
    if (sizeMatch.hasMatch())
        p.size = sizeMatch.captured(1).trimmed();

    // 匹配速度 (如 "at  2.50MiB/s")
    QRegularExpression speedRe("at\\s+([0-9.]+\\s*[KMG]?iB/s)");
    QRegularExpressionMatch speedMatch = speedRe.match(line);
    if (speedMatch.hasMatch())
        p.speed = speedMatch.captured(1).trimmed();

    // 匹配 ETA (如 "ETA 00:03" 或 "ETA Unknown")
    QRegularExpression etaRe("ETA\\s+([0-9:]+|Unknown)");
    QRegularExpressionMatch etaMatch = etaRe.match(line);
    if (etaMatch.hasMatch())
        p.eta = etaMatch.captured(1);

    // 完成状态：匹配 "in 00:04"
    QRegularExpression doneRe("in\\s+([0-9:]+)$");
    if (doneRe.match(line).hasMatch())
        p.eta = QString::fromUtf8("已完成");

    return p;
}

DownloadProgress DownloadProgressParser::parseAria2cLine(const QString &line) {
    DownloadProgress p;

    // 匹配百分比
    QRegularExpression pctRe("\\(([0-9.]+)%\\)");
    QRegularExpressionMatch pctMatch = pctRe.match(line);
    if (!pctMatch.hasMatch())
        return p;

    p.valid = true;
    p.percent = static_cast<int>(pctMatch.captured(1).toDouble());

    // 匹配已下载/总大小 (如 "1.2MiB/3.4GiB")
    QRegularExpression sizeRe("([0-9.]+\\s*[KMG]?i?B)/([0-9.]+\\s*[KMG]?i?B)");
    QRegularExpressionMatch sizeMatch = sizeRe.match(line);
    if (sizeMatch.hasMatch())
        p.size = sizeMatch.captured(1) + " / " + sizeMatch.captured(2);

    // 匹配下载速度 (如 "DL:2.5MiB")
    QRegularExpression speedRe("DL:([0-9.]+\\s*[KMG]?i?B)");
    QRegularExpressionMatch speedMatch = speedRe.match(line);
    if (speedMatch.hasMatch())
        p.speed = speedMatch.captured(1) + "/s";

    // 匹配 ETA (如 "ETA:30m" 或 "ETA:5s")
    QRegularExpression etaRe("ETA:([0-9hms]+)");
    QRegularExpressionMatch etaMatch = etaRe.match(line);
    if (etaMatch.hasMatch())
        p.eta = etaMatch.captured(1);

    // 匹配连接数 (如 "CN:16")
    QRegularExpression cnRe("CN:(\\d+)");
    QRegularExpressionMatch cnMatch = cnRe.match(line);
    if (cnMatch.hasMatch())
        p.connections = cnMatch.captured(1).toInt();

    return p;
}
