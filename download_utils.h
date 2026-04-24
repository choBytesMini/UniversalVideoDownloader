#ifndef DOWNLOAD_UTILS_H
#define DOWNLOAD_UTILS_H

#include <QString>
#include <QStringList>

// ============================================================
// 共享工具函数
// ============================================================
namespace DownloadUtils {

// URL 类型判断：是否为磁力链接或 BT 种子
bool isMagnetOrTorrent(const QString &url);

// 获取系统默认下载目录
QString defaultDownloadDir();

// 构建 aria2c 命令行参数（含所有优化参数）
QStringList buildAria2cArgs(const QString &outputDir, const QString &url);

// 构建 cookie 参数：若 browser 为空或 "无" 则返回空列表
QStringList buildCookieArgs(const QString &browser);

} // namespace DownloadUtils

// ============================================================
// 下载进度解析结果
// ============================================================
struct DownloadProgress {
    int percent = -1;
    QString size;        // 已下载/总大小 或 总大小
    QString speed;       // 下载速度
    QString eta;         // 剩余时间
    int connections = 0; // 连接数（仅 aria2c）
    bool valid = false;
};

// ============================================================
// 下载进度解析器（纯静态方法，无状态）
// ============================================================
class DownloadProgressParser {
public:
    // 解析单行 yt-dlp --newline 输出
    // 示例: "[download]  42.3% of  1.50MiB at  2.50MiB/s ETA 00:03"
    static DownloadProgress parseYtdlpLine(const QString &line);

    // 解析单行 aria2c --summary-interval=1 输出
    // 示例: "[#abc123 1.2MiB/3.4GiB(42%) CN:16 DL:2.5MiB ETA:30m]"
    static DownloadProgress parseAria2cLine(const QString &line);
};

#endif // DOWNLOAD_UTILS_H
