#include "apimanager.h"
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>
#include "logger.h"

ApiManager::ApiManager(QObject *parent)
    : QObject{parent}
{
    manager = new QNetworkAccessManager(this);
}

void ApiManager::setBilibiliHeaders(QNetworkRequest &request)
{
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    request.setRawHeader("Referer", "https://search.bilibili.com/");
    request.setRawHeader("Origin", "https://search.bilibili.com");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
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
    reply->setProperty("keywords", keywords);
    reply->setProperty("limit", limit);
    reply->setProperty("offset", offset);
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
    reply->setProperty("songId", songId);
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
    reply->setProperty("songId", songId);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onSongDetailReplyFinished(reply); });
}

void ApiManager::downloadImage(const QUrl &url)
{
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);
    reply->setProperty("url", url);
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
    reply->setProperty("songId", songId);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onSongUrlReplyFinished(reply); });
}

void ApiManager::onSearchReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logApi) << "NetEase search failed:" << reply->errorString();
        emit error(reply->errorString());
    } else {
        emit searchFinished(QJsonDocument::fromJson(reply->readAll()),
                            reply->property("keywords").toString(),
                            reply->property("limit").toInt(),
                            reply->property("offset").toInt());
    }
    reply->deleteLater();
}

void ApiManager::onLyricReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logApi) << "NetEase lyric failed:" << reply->errorString();
        emit error(reply->errorString());
    } else {
        emit lyricFinished(QJsonDocument::fromJson(reply->readAll()),
                           reply->property("songId").toLongLong());
    }
    reply->deleteLater();
}

void ApiManager::onSongDetailReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logApi) << "NetEase song detail failed:" << reply->errorString();
        emit error(reply->errorString());
    } else {
        emit songDetailFinished(QJsonDocument::fromJson(reply->readAll()),
                                reply->property("songId").toLongLong());
    }
    reply->deleteLater();
}

void ApiManager::onImageReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logApi) << "Image download failed:" << reply->errorString();
        emit error(reply->errorString());
    } else {
        emit imageDownloaded(reply->readAll(), reply->property("url").toUrl());
    }
    reply->deleteLater();
}

void ApiManager::onSongUrlReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logApi) << "NetEase songUrl failed:" << reply->errorString();
        emit error(reply->errorString());
    } else {
        QByteArray responseData = reply->readAll();
        QString onlineUrl = QString::fromUtf8(responseData);
        if (!onlineUrl.isEmpty()) {
            emit songUrlReady(QUrl(onlineUrl), reply->property("songId").toLongLong());
        } else {
            qCWarning(logApi) << "NetEase songUrl: empty body for songId"
                              << reply->property("songId").toLongLong();
            emit error("无法解析歌曲链接");
        }
    }
    reply->deleteLater();
}

// ==================== Bilibili API Implementation ====================

void ApiManager::searchBilibiliVideos(const QString &keywords, int page)
{
    // 使用备用的搜索API端点，更稳定
    QUrl url("https://api.bilibili.com/x/web-interface/search/all");
    QUrlQuery query;
    query.addQueryItem("keyword", keywords);
    query.addQueryItem("page", QString::number(page));
    query.addQueryItem("pagesize", "20");
    url.setQuery(query);

    qCDebug(logApi) << "Bilibili search:" << keywords << "page" << page << "->" << url.toString();

    QNetworkRequest request(url);
    setBilibiliHeaders(request);

    QNetworkReply *reply = manager->get(request);
    reply->setProperty("keywords", keywords);
    reply->setProperty("page", page);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onBilibiliSearchReplyFinished(reply); });
}

void ApiManager::getBilibiliVideoInfo(const QString &bvid)
{
    QUrl url(QString("https://api.bilibili.com/x/web-interface/view?bvid=%1").arg(bvid));

    QNetworkRequest request(url);
    setBilibiliHeaders(request);

    QNetworkReply *reply = manager->get(request);
    reply->setProperty("bvid", bvid);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onBilibiliVideoInfoReplyFinished(reply); });
}

void ApiManager::getBilibiliAudioUrl(const QString &bvid, qint64 cid)
{
    QUrl url(QString("https://api.bilibili.com/x/player/playurl?bvid=%1&cid=%2&fnval=16").arg(bvid).arg(cid));

    QNetworkRequest request(url);
    setBilibiliHeaders(request);

    QNetworkReply *reply = manager->get(request);
    reply->setProperty("bvid", bvid);
    reply->setProperty("cid", cid);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onBilibiliAudioUrlReplyFinished(reply); });
}

void ApiManager::downloadBilibiliImage(const QUrl &url)
{
    QNetworkRequest request(url);
    setBilibiliHeaders(request);

    QNetworkReply *reply = manager->get(request);
    reply->setProperty("url", url);
    connect(reply, &QNetworkReply::finished, this, [this, reply](){ onBilibiliImageReplyFinished(reply); });
}

void ApiManager::downloadBilibiliAudio(const QUrl &url, const QString &bvid)
{
    // 使用流式下载到临时文件，实现边下边播
    streamBilibiliAudio(url, bvid);
}

void ApiManager::onBilibiliSearchReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qCWarning(logApi).noquote() << "Bilibili search network error:" << reply->errorString()
                                    << "| HTTP" << httpStatus;

        // 如果是412错误（Precondition Failed），可能是被限制了
        if (httpStatus == 412) {
            emit error("Bilibili搜索被限制，请稍后再试");
        } else {
            emit error("Bilibili搜索失败: " + reply->errorString());
        }
    } else {
        QByteArray data = reply->readAll();
        qCDebug(logApi) << "Bilibili search response size:" << data.size();
        qCDebug(logApi).noquote() << "Bilibili search response preview:" << data.left(500);
        emit bilibiliSearchFinished(QJsonDocument::fromJson(data),
                                    reply->property("keywords").toString(),
                                    reply->property("page").toInt());
    }
    reply->deleteLater();
}

void ApiManager::onBilibiliVideoInfoReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logApi) << "Bilibili videoInfo failed:" << reply->errorString();
        emit error("获取Bilibili视频信息失败: " + reply->errorString());
    } else {
        emit bilibiliVideoInfoFinished(QJsonDocument::fromJson(reply->readAll()),
                                       reply->property("bvid").toString());
    }
    reply->deleteLater();
}

void ApiManager::onBilibiliAudioUrlReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logApi) << "Bilibili audioUrl failed:" << reply->errorString();
        emit error("获取Bilibili音频地址失败: " + reply->errorString());
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject rootObj = doc.object();

        if (rootObj.value("code").toInt() != 0) {
            qCWarning(logApi) << "Bilibili API error:" << rootObj.value("message").toString();
            emit error("Bilibili API错误: " + rootObj.value("message").toString());
            reply->deleteLater();
            return;
        }

        QJsonObject data = rootObj.value("data").toObject();
        QUrl audioUrl;
        QString backupUrl;

        // 尝试获取dash音频
        if (data.contains("dash")) {
            QJsonObject dash = data.value("dash").toObject();
            QJsonArray audioArray = dash.value("audio").toArray();
            if (!audioArray.isEmpty()) {
                // 选择最高音质的音频
                int bestId = 0;
                QString bestUrl;
                for (const QJsonValue &val : audioArray) {
                    QJsonObject audio = val.toObject();
                    int id = audio.value("id").toInt();
                    if (id > bestId) {
                        bestId = id;
                        bestUrl = audio.value("baseUrl").toString();
                    }
                }
                if (!bestUrl.isEmpty()) {
                    audioUrl = QUrl(bestUrl);
                }
                // 保存备用URL
                if (!audioArray.isEmpty()) {
                    backupUrl = audioArray.first().toObject().value("baseUrl").toString();
                }
            }
        }

        // 备用：获取durl
        if (audioUrl.isEmpty() && data.contains("durl")) {
            QJsonArray durlArray = data.value("durl").toArray();
            if (!durlArray.isEmpty()) {
                audioUrl = QUrl(durlArray.first().toObject().value("url").toString());
            }
        }

        if (!audioUrl.isEmpty()) {
            qCDebug(logApi).noquote() << "Bilibili audio URL ready:" << audioUrl.toString().left(100) << "...";
            emit bilibiliAudioUrlReady(audioUrl,
                                       reply->property("bvid").toString(),
                                       reply->property("cid").toLongLong());
        } else if (!backupUrl.isEmpty()) {
            qCDebug(logApi) << "Using backup Bilibili audio URL";
            emit bilibiliAudioUrlReady(QUrl(backupUrl),
                                       reply->property("bvid").toString(),
                                       reply->property("cid").toLongLong());
        } else {
            emit error("无法获取Bilibili音频地址");
        }
    }
    reply->deleteLater();
}

void ApiManager::onBilibiliImageReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(logApi) << "Bilibili image download failed:" << reply->errorString();
        emit error("下载Bilibili图片失败: " + reply->errorString());
    } else {
        emit bilibiliImageDownloaded(reply->readAll(), reply->property("url").toUrl());
    }
    reply->deleteLater();
}

void ApiManager::streamBilibiliAudio(const QUrl &url, const QString &bvid)
{
    QNetworkRequest request(url);
    setBilibiliHeaders(request);

    QNetworkReply *reply = manager->get(request);

    // 临时文件挂到 reply 上：reply 析构时 tempFile 一并析构。
    // 成功路径 setAutoRemove(false) 把磁盘文件交给 MusicController；失败/中途打断路径
    // 保持 autoRemove=true，tempFile 随 reply 删除时自动清理磁盘，杜绝泄漏。
    QTemporaryFile *tempFile = new QTemporaryFile(reply);
    if (!tempFile->open()) {
        qCWarning(logApi) << "Failed to create temp file for Bilibili audio stream";
        emit error("无法创建临时文件用于音频下载");
        reply->deleteLater(); // tempFile 随之析构
        return;
    }

    // 连接下载进度信号
    connect(reply, &QNetworkReply::downloadProgress, this, [reply, tempFile](qint64 bytesReceived, qint64 bytesTotal) {
        Q_UNUSED(bytesTotal)
        // 将接收到的数据写入临时文件
        tempFile->write(reply->readAll());
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, tempFile, bvid]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(logApi) << "Bilibili stream download failed:" << reply->errorString();
            emit error("流式下载Bilibili音频失败: " + reply->errorString());
            // autoRemove 默认 true，tempFile 随 reply 删除时自动删盘
        } else {
            // 写入剩余数据
            tempFile->write(reply->readAll());
            tempFile->flush();

            // 文件将由接收方管理，关闭自动删除后再交出路径
            tempFile->setAutoRemove(false);
            tempFile->close();
            emit bilibiliAudioFileReady(tempFile->fileName(), bvid);
        }
        reply->deleteLater(); // tempFile 随之析构（文件已 setAutoRemove(false)，不会被删）
    });
}
