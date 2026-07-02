#ifndef APPSTATESTORE_H
#define APPSTATESTORE_H

#include <QJsonObject>
#include <QSqlDatabase>
#include <QString>
#include <QVariantMap>
#include <QVector>
#include "playlistmanager.h"

struct SearchStateRecord
{
    QString keywords;
    int page = 1;
    int totalPages = 0;
    QVector<Song> songs;
};

class AppStateStore
{
public:
    AppStateStore();
    ~AppStateStore();

    bool open();

    QVariantMap loadSettings() const;
    void saveSetting(const QString &key, const QVariant &value);

    SearchStateRecord loadSearchState(SearchSource source) const;
    void saveSearchState(SearchSource source, const QString &keywords, int page, int totalPages, const QVector<Song> &songs);

    QVector<Song> loadQueue(int *currentIndex, int *playMode) const;
    void saveQueue(const QVector<Song> &songs, int currentIndex, int playMode);

private:
    bool exec(const QString &sql) const;
    static QJsonObject songToJson(const Song &song);
    static Song songFromJson(const QJsonObject &obj);

    QString m_connectionName;
    QSqlDatabase m_db;
};

#endif // APPSTATESTORE_H
