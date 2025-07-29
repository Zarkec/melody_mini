#ifndef APIMANAGER_H
#define APIMANAGER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrl>

class ApiManager : public QObject
{
    Q_OBJECT
public:
    explicit ApiManager(QObject *parent = nullptr);

    void searchSongs(const QString &keywords, int limit = 15, int offset = 0);
    void getLyric(qint64 songId);
    void getSongDetail(qint64 songId);
    void downloadImage(const QUrl &url);
    void getSongUrl(qint64 songId);

signals:
    void searchFinished(const QJsonDocument &json);
    void lyricFinished(const QJsonDocument &json);
    void songDetailFinished(const QJsonDocument &json);
    void imageDownloaded(const QByteArray &data);
    void songUrlReady(const QUrl &url);
    void error(const QString &errorString);

private slots:
    void onSearchReplyFinished(QNetworkReply *reply);
    void onLyricReplyFinished(QNetworkReply *reply);
    void onSongDetailReplyFinished(QNetworkReply *reply);
    void onImageReplyFinished(QNetworkReply *reply);
    void onSongUrlReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
};

#endif // APIMANAGER_H