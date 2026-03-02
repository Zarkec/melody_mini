#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>

// 搜索源类型
enum class SearchSource {
    NetEase,    // 网易云音乐
    Bilibili    // Bilibili
};

// 用于存储歌曲基本信息的结构体
struct Song
{
    qint64 id;
    QString name;
    QString artist;
    // Bilibili特有字段
    QString bvid;       // Bilibili BV号
    QString picUrl;     // 封面图URL
    qint64 cid;         // Bilibili CID
    int duration;       // 时长（秒）
    SearchSource source; // 来源平台

    Song() : id(-1), cid(-1), duration(0), source(SearchSource::NetEase) {}
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