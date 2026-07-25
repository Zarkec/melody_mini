#include "playlistmanager.h"
#include <QRandomGenerator>
#include "logger.h"

PlaylistManager::PlaylistManager(QObject *parent)
    : QObject(parent), currentIndex(-1), currentMode(Sequential)
{
}

// 设置播放列表
void PlaylistManager::addSongs(const QVector<Song> &songs)
{
    playlist = songs;
    currentIndex = -1; // 重置索引
    qCInfo(logPlaylist) << "Playlist replaced:" << playlist.size() << "songs";
}

// 设置当前播放歌曲的索引
void PlaylistManager::setCurrentIndex(int index)
{
    if (index >= 0 && index < playlist.size()) {
        currentIndex = index;
        qCDebug(logPlaylist) << "Current index set to" << currentIndex;
    }
}

// 获取下一首歌曲
Song PlaylistManager::getNextSong(bool isAutoTriggered)
{
    if (playlist.isEmpty()) {
        qCWarning(logPlaylist) << "getNextSong called on empty playlist";
        return Song(); // 返回无效歌曲
    }

    if (currentMode == LoopOne && isAutoTriggered) {
        // 单曲循环模式下，自动播放时索引不变
        qCDebug(logPlaylist) << "LoopOne(auto): keep index" << currentIndex;
        return playlist[currentIndex];
    }

    if (currentMode == Random) {
        if (playlist.size() > 1) {
            int newIndex;
            do {
                newIndex = QRandomGenerator::global()->bounded(playlist.size());
            } while (newIndex == currentIndex); // 避免随机到同一首歌
            currentIndex = newIndex;
        }
        // 如果只有一首歌，索引不变
        qCDebug(logPlaylist) << "Random: index ->" << currentIndex;
    } else { // Sequential or LoopOne (manual next)
        currentIndex = (currentIndex + 1) % playlist.size();
        qCDebug(logPlaylist) << "Sequential: index ->" << currentIndex;
    }

    return playlist[currentIndex];
}

// 获取上一首歌曲
Song PlaylistManager::getPreviousSong()
{
    if (playlist.isEmpty()) {
        qCWarning(logPlaylist) << "getPreviousSong called on empty playlist";
        return Song(); // 返回无效歌曲
    }

    currentIndex = (currentIndex - 1 + playlist.size()) % playlist.size();
    qCDebug(logPlaylist) << "Previous: index ->" << currentIndex;
    return playlist[currentIndex];
}

// 获取当前歌曲
Song PlaylistManager::getCurrentSong() const
{
    if (currentIndex >= 0 && currentIndex < playlist.size()) {
        return playlist[currentIndex];
    }
    return Song(); // 返回无效歌曲
}

// 设置播放模式
void PlaylistManager::setPlayMode(PlayMode mode)
{
    if (currentMode == mode) return;
    qCInfo(logPlaylist) << "PlayMode:" << currentMode << "->" << mode;
    currentMode = mode;
}

QVector<Song> PlaylistManager::songs() const
{
    return playlist;
}

void PlaylistManager::restoreSongs(const QVector<Song> &songs, int index)
{
    playlist = songs;
    currentIndex = (index >= 0 && index < playlist.size()) ? index : -1;
    qCInfo(logPlaylist) << "Playlist restored:" << playlist.size() << "songs, index" << currentIndex;
}

// 获取当前播放模式
PlaylistManager::PlayMode PlaylistManager::getPlayMode() const
{
    return currentMode;
}

// 获取当前索引
int PlaylistManager::getCurrentIndex() const
{
    return currentIndex;
}

// 判断列表是否为空
bool PlaylistManager::isEmpty() const
{
    return playlist.isEmpty();
}
