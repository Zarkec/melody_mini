#ifndef MUSICCONTROLLER_H
#define MUSICCONTROLLER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>
#include <QVector>
#include "playlistmanager.h"

class ApiManager;

class MusicController : public QObject
{
    Q_OBJECT

    // ---- 播放状态属性 ----
    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY hasMediaChanged)

    // ---- 当前歌曲信息 ----
    Q_PROPERTY(QString currentSongName READ currentSongName NOTIFY currentSongChanged)
    Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentSongChanged)
    Q_PROPERTY(QString currentLyric READ currentLyric NOTIFY lyricChanged)
    Q_PROPERTY(QUrl albumArtUrl READ albumArtUrl NOTIFY albumArtChanged)

    // ---- 搜索与分页 ----
    Q_PROPERTY(QVariantList searchResults READ searchResults NOTIFY searchResultsChanged)
    Q_PROPERTY(int currentPage READ currentPage NOTIFY pageChanged)
    Q_PROPERTY(int totalPages READ totalPages NOTIFY pageChanged)
    Q_PROPERTY(bool searchLoading READ isSearchLoading NOTIFY searchLoadingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool statusIsError READ statusIsError NOTIFY statusMessageChanged)
    Q_PROPERTY(int searchSource READ searchSource WRITE setSearchSource NOTIFY searchSourceChanged)

    // ---- 播放模式 ----
    Q_PROPERTY(int playMode READ playMode NOTIFY playModeChanged)

    // ---- 音频设备 ----
    Q_PROPERTY(QVariantList audioDevices READ audioDevices NOTIFY audioDevicesChanged)
    Q_PROPERTY(int currentAudioDeviceIndex READ currentAudioDeviceIndex NOTIFY audioDevicesChanged)

    // ---- 调色板颜色（用于动画背景） ----
    Q_PROPERTY(QVariantList paletteColors READ paletteColors NOTIFY paletteColorsChanged)

    // ---- Bilibili 分P选择 ----
    Q_PROPERTY(QVariantList bilibiliPages READ bilibiliPages NOTIFY bilipagesChanged)
    Q_PROPERTY(int currentBilibiliPage READ currentBilibiliPage NOTIFY bilipagesChanged)

public:
    explicit MusicController(QObject *parent = nullptr);
    ~MusicController();

    // Property getters
    bool isPlaying() const;
    qint64 position() const;
    qint64 duration() const;
    int volume() const;
    bool isLoading() const;
    bool hasMedia() const;
    QString currentSongName() const;
    QString currentArtist() const;
    QString currentLyric() const;
    QUrl albumArtUrl() const;
    QVariantList searchResults() const;
    int currentPage() const;
    int totalPages() const;
    bool isSearchLoading() const;
    QString statusMessage() const;
    bool statusIsError() const;
    int searchSource() const;
    int playMode() const;
    QVariantList audioDevices() const;
    int currentAudioDeviceIndex() const;
    QVariantList paletteColors() const;
    QVariantList bilibiliPages() const;
    int currentBilibiliPage() const;

    // Property setters
    void setVolume(int vol);
    void setSearchSource(int source);

public slots:
    // ---- QML invokable actions ----
    Q_INVOKABLE void search(const QString &keywords);
    Q_INVOKABLE void prevPage();
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void playSongAt(int index);
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void playNext();
    Q_INVOKABLE void playPrev();
    Q_INVOKABLE void cyclePlayMode();
    Q_INVOKABLE void seekTo(qint64 positionMs);
    Q_INVOKABLE void selectAudioDevice(int index);
    Q_INVOKABLE void selectBilibiliPage(int index);
    Q_INVOKABLE QString formatTime(qint64 ms) const;

signals:
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void loadingChanged();
    void hasMediaChanged();
    void currentSongChanged();
    void lyricChanged();
    void albumArtChanged();
    void searchResultsChanged();
    void pageChanged();
    void searchLoadingChanged();
    void statusMessageChanged();
    void searchSourceChanged();
    void playModeChanged();
    void audioDevicesChanged();
    void paletteColorsChanged();
    void bilipagesChanged();

private slots:
    // Network callbacks
    void onSearchFinished(const QJsonDocument &json, const QString &keywords, int limit, int offset);
    void onLyricFinished(const QJsonDocument &json, qint64 songId);
    void onSongDetailFinished(const QJsonDocument &json, qint64 songId);
    void onImageDownloaded(const QByteArray &data, const QUrl &url);
    void onSongUrlReady(const QUrl &url, qint64 songId);
    void onBilibiliSearchFinished(const QJsonDocument &json, const QString &keywords, int page);
    void onBilibiliVideoInfoFinished(const QJsonDocument &json, const QString &bvid);
    void onBilibiliAudioUrlReady(const QUrl &url, const QString &bvid, qint64 cid);
    void onBilibiliAudioFileReady(const QString &filePath);
    void onBilibiliImageDownloaded(const QByteArray &data, const QUrl &url);
    void onApiError(const QString &errorString);

    // Media player callbacks
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onMediaPlayerError(QMediaPlayer::Error error, const QString &errorString);
    void onAudioOutputsChanged();

private:
    void playSong(qint64 id);
    void playBilibiliVideo(const QString &bvid);
    void setPlaybackLoading(bool loading);
    void setSearchLoading(bool loading);
    void showStatus(const QString &message, bool isError = false);
    void searchCurrentPage();
    void cleanupTempAudio();
    void refreshAudioDevices();
    bool applyAudioDevice(const QByteArray &deviceId);
    void parseLyrics(const QString &lyricText);
    void updateCurrentLyric(qint64 positionMs);
    QVariantList extractPaletteColors(const QImage &image, int count = 3);

    // Media
    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    QMediaDevices *m_mediaDevices;

    // Backend
    ApiManager *m_api;
    PlaylistManager *m_playlist;

    // State
    bool m_searchLoading = false;
    bool m_playbackLoading = false;
    qint64 m_currentPlayingSongId = -1;
    QString m_currentBvid;
    QString m_currentSongName;
    QString m_currentArtist;
    QString m_currentLyric;
    QUrl m_albumArtUrl;
    QImage m_albumArtImage;
    QMap<qint64, QString> m_lyricData;
    QVariantList m_paletteColors;

    QString m_statusMessage;
    bool m_statusIsError = false;

    // Search state
    QVariantList m_searchResults;
    QVector<Song> m_searchSongs;
    QString m_currentKeywords;
    int m_currentPage = 1;
    int m_totalPages = 0;
    int m_searchSource = 0; // 0=NetEase, 1=Bilibili

    // Bilibili state
    QVariantList m_bilibiliPages;
    int m_currentBilibiliPageIndex = 0;
    qint64 m_currentBilibiliCid = -1;
    QUrl m_currentBilibiliAudioUrl;
    QString m_currentBilibiliAudioBvid;
    QUrl m_pendingCoverUrl;
    int m_pendingCoverSource = 0;

    // Audio devices
    QVariantList m_audioDevices;
    int m_currentAudioDeviceIndex = 0;
    bool m_userSelectedDevice = false;
    QByteArray m_selectedDeviceId;

    // Temp audio file
    QString m_tempAudioPath;
    QMetaObject::Connection m_tempCleanupConn;
};

#endif // MUSICCONTROLLER_H
