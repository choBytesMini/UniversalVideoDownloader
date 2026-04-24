#ifndef URL_EXTRACTOR_H
#define URL_EXTRACTOR_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class UrlExtractor : public QObject {
    Q_OBJECT

public:
    explicit UrlExtractor(QObject *parent = nullptr);

    void extract(const QString &url, const QString &cookieBrowser = QString());

private slots:
    void onFetchFinished(QNetworkReply *reply);

signals:
    void urlsFound(const QStringList &extractedUrls);
    void noUrlFound(const QString &pageUrl);
    void errorOccurred(const QString &errorMsg);

private:
    QNetworkAccessManager *networkManager;
};

#endif // URL_EXTRACTOR_H
