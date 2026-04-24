#include "url_extractor.h"
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>

UrlExtractor::UrlExtractor(QObject *parent) : QObject(parent) {
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &UrlExtractor::onFetchFinished);
}

void UrlExtractor::extract(const QString &url, const QString &cookieBrowser) {
    Q_UNUSED(cookieBrowser)

    QNetworkRequest request(url);
    // 伪造浏览器请求头，绕过简单防爬虫
    request.setRawHeader("User-Agent",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    request.setRawHeader("Accept",
        "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    request.setRawHeader("Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8");
    // 以当前 URL 作为 Referer，应对部分防盗链
    QUrl qurl(url);
    QString referer = qurl.scheme() + "://" + qurl.host();
    request.setRawHeader("Referer", referer.toUtf8());

    QNetworkReply *reply = networkManager->get(request);

    // 15 秒超时
    QTimer::singleShot(15000, reply, [this, reply]() {
        if (reply->isRunning()) {
            reply->abort();
            emit errorOccurred("网页抓取超时 (15s)");
        }
    });
}

// ============================================================
// 反转义：处理 JSON/JS 字符串中的转义序列
//   \/  → /
//   \\  → \
//   \"  → "
//   \n  → 换行
//   \u002F → /, \u003A → : 等 4 位十六进制 Unicode
// ============================================================
static QString unescapeJsonString(const QString &escaped) {
    QString result;
    result.reserve(escaped.size());

    for (int i = 0; i < escaped.size(); ++i) {
        if (escaped[i] == '\\' && i + 1 < escaped.size()) {
            QChar next = escaped[i + 1];
            switch (next.unicode()) {
            case '/':  result.append('/');  i++; break;
            case '\\': result.append('\\'); i++; break;
            case '"':  result.append('"');  i++; break;
            case 'n':  result.append('\n'); i++; break;
            case 't':  result.append('\t'); i++; break;
            case 'r':  result.append('\r'); i++; break;
            case 'u':
                // \uXXXX → 4 位十六进制 Unicode 码点
                if (i + 5 < escaped.size()) {
                    QString hex = escaped.mid(i + 2, 4);
                    bool ok = false;
                    ushort code = hex.toUShort(&ok, 16);
                    if (ok) {
                        result.append(QChar(code));
                        i += 5;
                    } else {
                        result.append(escaped[i]);
                    }
                } else {
                    result.append(escaped[i]);
                }
                break;
            default:
                // 未知转义符，保留原字符
                result.append(next);
                i++;
                break;
            }
        } else {
            result.append(escaped[i]);
        }
    }
    return result;
}

// ============================================================
// 辅助：从引号包裹的 value 中提取含指定后缀的 URL
// 匹配形如 "key":"value" 或 "key": "value"，其中 value 含目标后缀
// 同时处理转义形式（\/ 代替 /）
// ============================================================
static QStringList findQuotedUrlsBySuffix(const QString &html,
                                     const QString &suffix,
                                     bool unescape) {
    QStringList results;
    QString escapedSuffix;
    for (const QChar &c : suffix) {
        if (c == '.') {
            escapedSuffix += "\\.";
        } else {
            escapedSuffix += c;
        }
    }

    QString pattern = "\"(?:url|file|src|hlsUrl|playUrl|videoUrl|stream_url|manifest_url|dashUrl)\""
                      "\\s*:\\s*\"([^\"]*" + escapedSuffix + "[^\"]*)\"";
    QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = re.globalMatch(html);
    while (it.hasNext()) {
        QString raw = it.next().captured(1);
        QString clean = unescape ? unescapeJsonString(raw) : raw;
        if (clean.startsWith("http")) {
            results.append(clean);
        }
    }
    return results;
}

// ============================================================
// 从原始 HTML 源码中提取媒体流地址（m3u8 / mpd / mp4 等）
//
// 提取策略（按优先级）：
// 1. JS/JSON 转义字符串中的 URL（如 "url":"https:\/\/...\.m3u8"）
// 2. 普通裸 URL（如 https://...m3u8?token=xxx）
// 3. HTML 标签中的 src 属性（<video>/<source>）
// ============================================================
static QStringList extractMediaUrls(const QString &html) {
    QStringList urls;

    // ---- 第 1 层：匹配 JS/JSON 中转义的 m3u8 ----
    urls.append(findQuotedUrlsBySuffix(html, ".m3u8", true));

    // ---- 第 2 层：匹配 JS/JSON 中转义的 mpd ----
    urls.append(findQuotedUrlsBySuffix(html, ".mpd", true));

    // ---- 第 3 层：匹配 JS/JSON 中转义的 mp4 ----
    urls.append(findQuotedUrlsBySuffix(html, ".mp4", true));

    // ---- 第 4 层：裸 URL（未转义），直接出现在 HTML/JS 文本中 ----
    // m3u8
    {
        QRegularExpression re("https?://[^\\s\"'<>]+\\.m3u8[^\\s\"'<>]*",
                              QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = re.globalMatch(html);
        while (it.hasNext()) urls.append(it.next().captured());
    }

    // mpd
    {
        QRegularExpression re("https?://[^\\s\"'<>]+\\.mpd[^\\s\"'<>]*",
                              QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = re.globalMatch(html);
        while (it.hasNext()) urls.append(it.next().captured());
    }

    // mp4 / webm / flv
    {
        QRegularExpression re("https?://[^\\s\"'<>]+\\.(?:mp4|webm|flv|mkv)(?:\\?[^\\s\"'<>]*)?",
                              QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = re.globalMatch(html);
        while (it.hasNext()) urls.append(it.next().captured());
    }

    // ---- 第 5 层：HTML 标签 src 属性 ----
    {
        QRegularExpression re("<(?:video|source)[^>]+src\\s*=\\s*[\"']([^\"']+)[\"']",
                              QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator it = re.globalMatch(html);
        while (it.hasNext()) urls.append(it.next().captured(1));
    }

    urls.removeDuplicates();
    return urls;
}

void UrlExtractor::onFetchFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            emit errorOccurred(QString("网页抓取失败: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
        return;
    }

    QString html = QString::fromUtf8(reply->readAll());
    QString pageUrl = reply->url().toString();
    reply->deleteLater();

    if (html.isEmpty()) {
        emit noUrlFound(pageUrl);
        return;
    }

    QStringList foundUrls = extractMediaUrls(html);
    if (!foundUrls.isEmpty()) {
        emit urlsFound(foundUrls);
    } else {
        emit noUrlFound(pageUrl);
    }
}
