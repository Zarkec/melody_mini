#include "appstatestore.h"
#include "logger.h"

#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

AppStateStore::AppStateStore()
    : m_connectionName(QStringLiteral("melody-state-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

AppStateStore::~AppStateStore()
{
    if (m_db.isValid()) {
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool AppStateStore::open()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        qCWarning(logPlayer) << "App state path is empty";
        return false;
    }
    if (!QDir().mkpath(dir)) {
        qCWarning(logPlayer).noquote() << "Failed to create app state dir:" << dir;
        return false;
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dir + QStringLiteral("/melody_state.sqlite"));
    if (!m_db.open()) {
        qCWarning(logPlayer) << "Failed to open app state db:" << m_db.lastError().text();
        return false;
    }

    return exec(QStringLiteral("CREATE TABLE IF NOT EXISTS settings ("
                               "key TEXT PRIMARY KEY, "
                               "value TEXT NOT NULL)"))
        && exec(QStringLiteral("CREATE TABLE IF NOT EXISTS search_state ("
                               "source INTEGER PRIMARY KEY, "
                               "keywords TEXT NOT NULL, "
                               "page INTEGER NOT NULL DEFAULT 1, "
                               "total_pages INTEGER NOT NULL DEFAULT 0, "
                               "results_json TEXT NOT NULL DEFAULT '[]', "
                               "updated_at INTEGER NOT NULL)"))
        && exec(QStringLiteral("CREATE TABLE IF NOT EXISTS playback_queue ("
                               "position INTEGER PRIMARY KEY, "
                               "song_json TEXT NOT NULL)"))
        && exec(QStringLiteral("CREATE TABLE IF NOT EXISTS playback_state ("
                               "id INTEGER PRIMARY KEY CHECK (id = 1), "
                               "current_index INTEGER NOT NULL DEFAULT -1, "
                               "play_mode INTEGER NOT NULL DEFAULT 0)"));
}

QVariantMap AppStateStore::loadSettings() const
{
    QVariantMap settings;
    if (!m_db.isOpen()) return settings;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT key, value FROM settings"))) {
        qCWarning(logPlayer) << "Failed to load settings:" << query.lastError().text();
        return settings;
    }
    while (query.next())
        settings.insert(query.value(0).toString(), query.value(1));
    return settings;
}

void AppStateStore::saveSetting(const QString &key, const QVariant &value)
{
    if (!m_db.isOpen()) return;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO settings(key, value) VALUES(?, ?)"));
    query.addBindValue(key);
    query.addBindValue(value.toString());
    if (!query.exec())
        qCWarning(logPlayer) << "Failed to save setting" << key << ":" << query.lastError().text();
}

SearchStateRecord AppStateStore::loadSearchState(SearchSource source) const
{
    SearchStateRecord record;
    if (!m_db.isOpen()) return record;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT keywords, page, total_pages, results_json FROM search_state WHERE source = ?"));
    query.addBindValue(static_cast<int>(source));
    if (!query.exec()) {
        qCWarning(logPlayer) << "Failed to load search state:" << query.lastError().text();
        return record;
    }
    if (!query.next()) return record;

    record.keywords = query.value(0).toString();
    record.page = qMax(1, query.value(1).toInt());
    record.totalPages = qMax(0, query.value(2).toInt());

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(query.value(3).toString().toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) return record;
    for (const QJsonValue &value : doc.array()) {
        if (value.isObject())
            record.songs.append(songFromJson(value.toObject()));
    }
    return record;
}

void AppStateStore::saveSearchState(SearchSource source, const QString &keywords, int page, int totalPages, const QVector<Song> &songs)
{
    if (!m_db.isOpen()) return;

    QJsonArray array;
    for (const Song &song : songs)
        array.append(songToJson(song));

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO search_state(source, keywords, page, total_pages, results_json, updated_at) "
                                 "VALUES(?, ?, ?, ?, ?, ?)"));
    query.addBindValue(static_cast<int>(source));
    query.addBindValue(keywords);
    query.addBindValue(qMax(1, page));
    query.addBindValue(qMax(0, totalPages));
    query.addBindValue(QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact)));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec())
        qCWarning(logPlayer) << "Failed to save search state:" << query.lastError().text();
}

QVector<Song> AppStateStore::loadQueue(int *currentIndex, int *playMode) const
{
    QVector<Song> songs;
    if (currentIndex) *currentIndex = -1;
    if (playMode) *playMode = 0;
    if (!m_db.isOpen()) return songs;

    QSqlQuery stateQuery(m_db);
    if (stateQuery.exec(QStringLiteral("SELECT current_index, play_mode FROM playback_state WHERE id = 1")) && stateQuery.next()) {
        if (currentIndex) *currentIndex = stateQuery.value(0).toInt();
        if (playMode) *playMode = stateQuery.value(1).toInt();
    }

    QSqlQuery queueQuery(m_db);
    if (!queueQuery.exec(QStringLiteral("SELECT song_json FROM playback_queue ORDER BY position ASC"))) {
        qCWarning(logPlayer) << "Failed to load queue:" << queueQuery.lastError().text();
        return songs;
    }
    while (queueQuery.next()) {
        QJsonDocument doc = QJsonDocument::fromJson(queueQuery.value(0).toString().toUtf8());
        if (doc.isObject())
            songs.append(songFromJson(doc.object()));
    }
    return songs;
}

void AppStateStore::saveQueue(const QVector<Song> &songs, int currentIndex, int playMode)
{
    if (!m_db.isOpen()) return;

    if (!m_db.transaction())
        qCWarning(logPlayer) << "Failed to begin queue save transaction:" << m_db.lastError().text();

    QSqlQuery clearQuery(m_db);
    if (!clearQuery.exec(QStringLiteral("DELETE FROM playback_queue")))
        qCWarning(logPlayer) << "Failed to clear queue:" << clearQuery.lastError().text();

    QSqlQuery insertQuery(m_db);
    insertQuery.prepare(QStringLiteral("INSERT INTO playback_queue(position, song_json) VALUES(?, ?)"));
    for (int i = 0; i < songs.size(); ++i) {
        insertQuery.addBindValue(i);
        insertQuery.addBindValue(QString::fromUtf8(QJsonDocument(songToJson(songs.at(i))).toJson(QJsonDocument::Compact)));
        if (!insertQuery.exec())
            qCWarning(logPlayer) << "Failed to save queue item:" << insertQuery.lastError().text();
    }

    QSqlQuery stateQuery(m_db);
    stateQuery.prepare(QStringLiteral("INSERT OR REPLACE INTO playback_state(id, current_index, play_mode) VALUES(1, ?, ?)"));
    stateQuery.addBindValue(currentIndex);
    stateQuery.addBindValue(playMode);
    if (!stateQuery.exec())
        qCWarning(logPlayer) << "Failed to save playback state:" << stateQuery.lastError().text();

    if (!m_db.commit())
        qCWarning(logPlayer) << "Failed to commit queue save transaction:" << m_db.lastError().text();
}

bool AppStateStore::exec(const QString &sql) const
{
    QSqlQuery query(m_db);
    if (query.exec(sql)) return true;
    qCWarning(logPlayer) << "App state schema error:" << query.lastError().text();
    return false;
}

QJsonObject AppStateStore::songToJson(const Song &song)
{
    QJsonObject obj;
    obj["id"] = QString::number(song.id);
    obj["name"] = song.name;
    obj["artist"] = song.artist;
    obj["bvid"] = song.bvid;
    obj["picUrl"] = song.picUrl;
    obj["cid"] = QString::number(song.cid);
    obj["duration"] = song.duration;
    obj["source"] = static_cast<int>(song.source);
    return obj;
}

Song AppStateStore::songFromJson(const QJsonObject &obj)
{
    Song song;
    song.id = obj.value("id").toVariant().toLongLong();
    song.name = obj.value("name").toString();
    song.artist = obj.value("artist").toString();
    song.bvid = obj.value("bvid").toString();
    song.picUrl = obj.value("picUrl").toString();
    song.cid = obj.value("cid").toVariant().toLongLong();
    song.duration = obj.value("duration").toInt();
    song.source = obj.value("source").toInt() == 1 ? SearchSource::Bilibili : SearchSource::NetEase;
    return song;
}
