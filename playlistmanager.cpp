#include "playlistmanager.h"
#include <QRandomGenerator>

PlaylistManager::PlaylistManager(QObject *parent)
    : QObject(parent), currentIndex(-1), currentMode(Sequential)
{
}

// 设置播放列表
void PlaylistManager::addSongs(const QVector<Song> &songs)
{
    playlist = songs;
    currentIndex = -1; // 重置索引
}

// 设置当前播放歌曲的索引
void PlaylistManager::setCurrentIndex(int index)
{
    if (index >= 0 && index < playlist.size()) {
        currentIndex = index;
    }
}

// 获取下一首歌曲
Song PlaylistManager::getNextSong(bool isAutoTriggered)
{
    if (playlist.isEmpty()) {
        return {-1, "", ""}; // 返回无效歌曲
    }

    if (currentMode == LoopOne && isAutoTriggered) {
        // 单曲循环模式下，自动播放时索引不变
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
    } else { // Sequential or LoopOne (manual next)
        currentIndex = (currentIndex + 1) % playlist.size();
    }

    return playlist[currentIndex];
}

// 获取上一首歌曲
Song PlaylistManager::getPreviousSong()
{
    if (playlist.isEmpty()) {
        return {-1, "", ""}; // 返回无效歌曲
    }

    // 随机模式下，上一曲通常表现为顺序播放的上一曲
    if (currentMode == Random) {
         currentIndex = (currentIndex - 1 + playlist.size()) % playlist.size();
    } else { // Sequential or LoopOne
        currentIndex = (currentIndex - 1 + playlist.size()) % playlist.size();
    }
    
    return playlist[currentIndex];
}

// 获取当前歌曲
Song PlaylistManager::getCurrentSong() const
{
    if (currentIndex >= 0 && currentIndex < playlist.size()) {
        return playlist[currentIndex];
    }
    return {-1, "", ""}; // 返回无效歌曲
}

// 设置播放模式
void PlaylistManager::setPlayMode(PlayMode mode)
{
    currentMode = mode;
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