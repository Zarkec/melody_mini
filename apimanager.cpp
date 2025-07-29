#include "apimanager.h"
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

ApiManager::ApiManager(QObject *parent)
    : QObject{parent}
{
    manager = new QNetworkAccessManager(this);
}

void ApiManager::searchSongs(const QString &keywords, int limit, int offset)
{
    QUrl url("https://music.163.com/api/search/get");
    QUrlQuery query;
    query.addQueryItem("s", keywords);
    query.addQueryItem("type", "1");
    query.addQueryItem("limit", QString::number(limit));
    query.addQueryItem("offset", QString::number(offset));
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onSearchReplyFinished(reply); });
}

void ApiManager::getLyric(qint64 songId)
{
    QUrl url("https://music.163.com/api/song/lyric");
    QUrlQuery query;
    query.addQueryItem("os", "pc");
    query.addQueryItem("id", QString::number(songId));
    query.addQueryItem("lv", "-1");
    query.addQueryItem("tv", "-1");
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onLyricReplyFinished(reply); });
}

void ApiManager::getSongDetail(qint64 songId)
{
    QUrl url("https://music.163.com/api/song/detail");
    QUrlQuery query;
    query.addQueryItem("ids", QString("[%1]").arg(songId));
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onSongDetailReplyFinished(reply); });
}

void ApiManager::downloadImage(const QUrl &url)
{
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onImageReplyFinished(reply); });
}

void ApiManager::getSongUrl(qint64 songId)
{
    QUrl url("https://musicbox-web-api.mu-jie.cc/wyy/mp3");
    QUrlQuery query;
    query.addQueryItem("rid", QString::number(songId));
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onSongUrlReplyFinished(reply); });
}

void ApiManager::onSearchReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
    } else {
        emit searchFinished(QJsonDocument::fromJson(reply->readAll()));
    }
    reply->deleteLater();
}

void ApiManager::onLyricReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
    } else {
        emit lyricFinished(QJsonDocument::fromJson(reply->readAll()));
    }
    reply->deleteLater();
}

void ApiManager::onSongDetailReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
    } else {
        emit songDetailFinished(QJsonDocument::fromJson(reply->readAll()));
    }
    reply->deleteLater();
}

void ApiManager::onImageReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
    } else {
        emit imageDownloaded(reply->readAll());
    }
    reply->deleteLater();
}

void ApiManager::onSongUrlReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit error(reply->errorString());
    } else {
        QByteArray responseData = reply->readAll();
        QString onlineUrl = QString::fromUtf8(responseData);
        if (!onlineUrl.isEmpty()) {
            emit songUrlReady(QUrl(onlineUrl));
        } else {
            emit error("无法解析歌曲链接");
        }
    }
    reply->deleteLater();
}
