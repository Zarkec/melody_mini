#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>

// 用于存储歌曲基本信息的结构体
struct Song
{
    qint64 id;
    QString name;
    QString artist;
};

class PlaylistManager : public QObject
{
    Q_OBJECT
public:
    // 定义播放模式的枚举
    enum PlayMode {
        Sequential, // 顺序播放
        LoopOne,    // 单曲循环
        Random      // 随机播放
    };
    Q_ENUM(PlayMode)

    explicit PlaylistManager(QObject *parent = nullptr);

    // 公共接口
    void addSongs(const QVector<Song> &songs);
    void setCurrentIndex(int index);
    void setPlayMode(PlayMode mode);

    Song getNextSong(bool isAutoTriggered = true); // isAutoTriggered 用于区分是自动播放下一首还是手动点击
    Song getPreviousSong();
    Song getCurrentSong() const;
    PlayMode getPlayMode() const;
    int getCurrentIndex() const;
    bool isEmpty() const;


private:
    QVector<Song> playlist;     // 歌曲列表
    int currentIndex;           // 当前播放索引
    PlayMode currentMode;       // 当前播放模式
};

#endif // PLAYLISTMANAGER_H