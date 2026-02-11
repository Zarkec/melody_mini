#ifndef APIMANAGER_H
#define APIMANAGER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrl>

// Bilibili视频信息结构体
struct BilibiliVideo {
    QString bvid;
    QString title;
    QString author;
    QString pic;      // 封面图URL
    qint64 cid;       // 用于获取播放地址
    int duration;     // 时长（秒）
};

class ApiManager : public QObject
{
    Q_OBJECT
public:
    explicit ApiManager(QObject *parent = nullptr);

    // 网易云音乐API
    void searchSongs(const QString &keywords, int limit = 15, int offset = 0);
    void getLyric(qint64 songId);
    void getSongDetail(qint64 songId);
    void downloadImage(const QUrl &url);
    void getSongUrl(qint64 songId);

    // Bilibili API
    void searchBilibiliVideos(const QString &keywords, int page = 1);
    void getBilibiliVideoInfo(const QString &bvid);
    void getBilibiliAudioUrl(const QString &bvid, qint64 cid);
    void downloadBilibiliImage(const QUrl &url);

signals:
    // 网易云音乐信号
    void searchFinished(const QJsonDocument &json);
    void lyricFinished(const QJsonDocument &json);
    void songDetailFinished(const QJsonDocument &json);
    void imageDownloaded(const QByteArray &data);
    void songUrlReady(const QUrl &url);

    // Bilibili信号
    void bilibiliSearchFinished(const QJsonDocument &json);
    void bilibiliVideoInfoFinished(const QJsonDocument &json);
    void bilibiliAudioUrlReady(const QUrl &url);
    void bilibiliImageDownloaded(const QByteArray &data);

    void error(const QString &errorString);

private slots:
    void onSearchReplyFinished(QNetworkReply *reply);
    void onLyricReplyFinished(QNetworkReply *reply);
    void onSongDetailReplyFinished(QNetworkReply *reply);
    void onImageReplyFinished(QNetworkReply *reply);
    void onSongUrlReplyFinished(QNetworkReply *reply);

    // Bilibili slots
    void onBilibiliSearchReplyFinished(QNetworkReply *reply);
    void onBilibiliVideoInfoReplyFinished(QNetworkReply *reply);
    void onBilibiliAudioUrlReplyFinished(QNetworkReply *reply);
    void onBilibiliImageReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;

    // Bilibili请求头
    void setBilibiliHeaders(QNetworkRequest &request);
};

#endif // APIMANAGER_H