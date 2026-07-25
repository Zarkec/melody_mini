#include "musiccontroller.h"
#include "apimanager.h"
#include "appstatestore.h"

#include <QMediaPlayer>
#include <QDir>
#include <QDateTime>
#include <QAudioOutput>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>
#include <QFile>
#include <algorithm>
#include "logger.h"

MusicController::MusicController(QObject *parent)
    : QObject(parent)
{
    // Media player
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaDevices = new QMediaDevices(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.5f);

    QAudioDevice defaultDev = QMediaDevices::defaultAudioOutput();
    if (!defaultDev.isNull())
        m_audioOutput->setDevice(defaultDev);
    refreshAudioDevices();

    // API + playlist
    m_api = new ApiManager(this);
    m_playlist = new PlaylistManager(this);
    m_stateStore = std::make_unique<AppStateStore>();

    // Media player signals
    connect(m_player, &QMediaPlayer::positionChanged, this, &MusicController::onPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MusicController::onDurationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &MusicController::onPlaybackStateChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MusicController::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &MusicController::onMediaPlayerError);

    // Audio device change
    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &MusicController::onAudioOutputsChanged);

    // API signals — NetEase
    connect(m_api, &ApiManager::searchFinished, this, &MusicController::onSearchFinished);
    connect(m_api, &ApiManager::lyricFinished, this, &MusicController::onLyricFinished);
    connect(m_api, &ApiManager::songDetailFinished, this, &MusicController::onSongDetailFinished);
    connect(m_api, &ApiManager::imageDownloaded, this, &MusicController::onImageDownloaded);
    connect(m_api, &ApiManager::songUrlReady, this, &MusicController::onSongUrlReady);

    // API signals — Bilibili
    connect(m_api, &ApiManager::bilibiliSearchFinished, this, &MusicController::onBilibiliSearchFinished);
    connect(m_api, &ApiManager::bilibiliVideoInfoFinished, this, &MusicController::onBilibiliVideoInfoFinished);
    connect(m_api, &ApiManager::bilibiliAudioUrlReady, this, &MusicController::onBilibiliAudioUrlReady);
    connect(m_api, &ApiManager::bilibiliAudioFileReady, this, &MusicController::onBilibiliAudioFileReady);
    connect(m_api, &ApiManager::bilibiliImageDownloaded, this, &MusicController::onBilibiliImageDownloaded);
    connect(m_api, &ApiManager::error, this, &MusicController::onApiError);

    loadPersistedState();
}

MusicController::~MusicController()
{
    cleanupTempAudio();
}

// ============================================================
// Property getters
// ============================================================

bool MusicController::isPlaying() const { return m_player->playbackState() == QMediaPlayer::PlayingState; }
qint64 MusicController::position() const { return m_player->position(); }
qint64 MusicController::duration() const { return m_player->duration(); }
int MusicController::volume() const { return qRound(m_audioOutput->volume() * 100); }
bool MusicController::isLoading() const { return m_playbackLoading; }
bool MusicController::hasMedia() const {
    return m_currentPlayingSongId != -1 || !m_currentBvid.isEmpty() || m_player->source().isValid();
}
QString MusicController::currentSongName() const { return m_currentSongName; }
QString MusicController::currentArtist() const { return m_currentArtist; }
QString MusicController::currentLyric() const { return m_currentLyric; }
QUrl MusicController::albumArtUrl() const { return m_albumArtUrl; }
QVariantList MusicController::searchResults() const { return activeSearchState().results; }
int MusicController::currentPage() const { return activeSearchState().page; }
int MusicController::totalPages() const { return activeSearchState().totalPages; }
bool MusicController::isSearchLoading() const { return m_searchLoading; }
QString MusicController::statusMessage() const { return m_statusMessage; }
bool MusicController::statusIsError() const { return m_statusIsError; }
int MusicController::searchSource() const { return m_searchSource; }
QString MusicController::currentKeywords() const { return activeSearchState().keywords; }
int MusicController::playMode() const { return static_cast<int>(m_playlist->getPlayMode()); }
QVariantList MusicController::audioDevices() const { return m_audioDevices; }
int MusicController::currentAudioDeviceIndex() const { return m_currentAudioDeviceIndex; }
QVariantList MusicController::paletteColors() const { return m_paletteColors; }
QVariantList MusicController::bilibiliPages() const { return m_bilibiliPages; }
int MusicController::currentBilibiliPage() const { return m_currentBilibiliPageIndex; }

// ============================================================
// Property setters
// ============================================================

void MusicController::setVolume(int vol)
{
    float v = qBound(0, vol, 100) / 100.0f;
    if (qFuzzyCompare(m_audioOutput->volume(), v)) return;
    m_audioOutput->setVolume(v);
    if (m_stateStore)
        m_stateStore->saveSetting(QStringLiteral("volume"), qBound(0, vol, 100));
    emit volumeChanged();
}

void MusicController::setSearchSource(int source)
{
    source = qBound(0, source, 1);
    if (m_searchSource == source) return;
    qCInfo(logPlayer) << "Search source:" << m_searchSource << "->" << source
                      << "(" << (source == 0 ? "NetEase" : "Bilibili") << ")";
    m_searchSource = source;
    if (m_stateStore)
        m_stateStore->saveSetting(QStringLiteral("currentSearchSource"), source);
    emit searchResultsChanged();
    emit pageChanged();
    emit searchSourceChanged();
}

// ============================================================
// QML invokable
// ============================================================

void MusicController::search(const QString &keywords)
{
    SearchState &state = activeSearchState();
    state.keywords = keywords.trimmed();
    if (state.keywords.isEmpty()) {
        state.results.clear();
        state.songs.clear();
        state.page = 1;
        state.totalPages = 0;
        saveCurrentSearchState();
        emit currentKeywordsChanged();
        emit searchResultsChanged();
        emit pageChanged();
        showStatus(QString());
        return;
    }
    state.page = 1;
    emit currentKeywordsChanged();
    emit pageChanged();
    setSearchLoading(true);
    showStatus(QString());
    searchCurrentPage();
}

void MusicController::prevPage()
{
    SearchState &state = activeSearchState();
    if (state.keywords.isEmpty() || state.totalPages <= 0 || state.page <= 1) return;
    state.page--;
    emit pageChanged();
    setSearchLoading(true);
    searchCurrentPage();
}

void MusicController::nextPage()
{
    SearchState &state = activeSearchState();
    if (state.keywords.isEmpty() || state.totalPages <= 0 || state.page >= state.totalPages) return;
    state.page++;
    emit pageChanged();
    setSearchLoading(true);
    searchCurrentPage();
}

void MusicController::playSongAt(int index)
{
    SearchState &state = activeSearchState();
    if (index < 0 || index >= state.songs.size()) return;
    const Song &clicked = state.songs.at(index);

    bool isSame = (clicked.source == SearchSource::NetEase && clicked.id == m_currentPlayingSongId) ||
                  (clicked.source == SearchSource::Bilibili && clicked.bvid == m_currentBvid);

    if (isSame && m_player->playbackState() != QMediaPlayer::StoppedState) {
        return; // already playing, just stay on player view
    }

    m_playlist->addSongs(state.songs);
    m_playlist->setCurrentIndex(index);
    savePlaybackState();
    emit hasMediaChanged();

    Song cur = m_playlist->getCurrentSong();
    if (cur.source == SearchSource::Bilibili)
        playBilibiliVideo(cur.bvid);
    else if (cur.id != -1)
        playSong(cur.id);
}

void MusicController::playPause()
{
    if (m_playbackLoading) return;
    if (!hasMedia()) return;
    if (!m_player->source().isValid()) {
        if (!m_currentBvid.isEmpty())
            playBilibiliVideo(m_currentBvid);
        else if (m_currentPlayingSongId != -1)
            playSong(m_currentPlayingSongId);
        return;
    }
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();
    else
        m_player->play();
}

void MusicController::playNext()
{
    if (m_playbackLoading || m_playlist->isEmpty()) return;
    savePlaybackState();

    // Bilibili多P视频：优先播放下一个分P
    if (!m_currentBvid.isEmpty() && m_bilibiliPages.size() > 1) {
        int nextPage = m_currentBilibiliPageIndex + 1;
        if (nextPage < m_bilibiliPages.size()) {
            selectBilibiliPage(nextPage);
            return;
        }
    }

    Song next = m_playlist->getNextSong();
    if (next.source == SearchSource::Bilibili && !next.bvid.isEmpty())
        playBilibiliVideo(next.bvid);
    else if (next.id != -1)
        playSong(next.id);
}

void MusicController::playPrev()
{
    if (m_playbackLoading || m_playlist->isEmpty()) return;
    savePlaybackState();

    // Bilibili多P视频：优先播放上一个分P
    if (!m_currentBvid.isEmpty() && m_bilibiliPages.size() > 1) {
        int prevPage = m_currentBilibiliPageIndex - 1;
        if (prevPage >= 0) {
            selectBilibiliPage(prevPage);
            return;
        }
    }

    Song prev = m_playlist->getPreviousSong();
    if (prev.source == SearchSource::Bilibili && !prev.bvid.isEmpty())
        playBilibiliVideo(prev.bvid);
    else if (prev.id != -1)
        playSong(prev.id);
}

void MusicController::cyclePlayMode()
{
    int next = (static_cast<int>(m_playlist->getPlayMode()) + 1) % 3;
    m_playlist->setPlayMode(static_cast<PlaylistManager::PlayMode>(next));
    savePlaybackState();
    emit playModeChanged();
}

void MusicController::seekTo(qint64 positionMs)
{
    if (m_player->duration() <= 0) return;
    m_player->setPosition(positionMs);
}

void MusicController::selectAudioDevice(int index)
{
    if (index < 0 || index >= m_audioDevices.size()) return;
    QByteArray id = m_audioDevices.at(index).toMap().value("id").toByteArray();
    if (applyAudioDevice(id)) {
        m_userSelectedDevice = true;
        m_selectedDeviceId = id;
        m_currentAudioDeviceIndex = index;
        if (m_stateStore)
            m_stateStore->saveSetting(QStringLiteral("selectedAudioDeviceId"), QString::fromLatin1(id.toBase64()));
        qCInfo(logPlayer).noquote() << "Audio device selected: index" << index
                                    << "|" << m_audioDevices.at(index).toMap().value("name").toString();
        emit audioDevicesChanged();
    } else {
        qCWarning(logPlayer) << "Failed to apply audio device at index" << index;
    }
}

void MusicController::selectBilibiliPage(int index)
{
    if (index < 0 || index >= m_bilibiliPages.size() || m_currentBvid.isEmpty()) return;
    qint64 cid = m_bilibiliPages.at(index).toMap().value("cid").toLongLong();
    m_currentBilibiliCid = cid;
    m_currentBilibiliPageIndex = index;
    setPlaybackLoading(true);
    m_api->getBilibiliAudioUrl(m_currentBvid, cid);
    emit bilipagesChanged();
}

QString MusicController::formatTime(qint64 ms) const
{
    qint64 s = ms / 1000;
    return QString("%1:%2").arg(s / 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
}

// ============================================================
// Private helpers
// ============================================================

void MusicController::searchCurrentPage()
{
    const SearchState &state = activeSearchState();
    if (m_searchSource == 0)
        m_api->searchSongs(state.keywords, 15, (state.page - 1) * 15);
    else
        m_api->searchBilibiliVideos(state.keywords, state.page);
}

void MusicController::loadPersistedState()
{
    if (!m_stateStore || !m_stateStore->open()) return;

    QVariantMap settings = m_stateStore->loadSettings();

    for (int i = 0; i < 2; ++i) {
        SearchStateRecord record = m_stateStore->loadSearchState(static_cast<SearchSource>(i));
        SearchState &state = m_searchStates[i];
        state.keywords = record.keywords;
        state.page = record.page;
        state.totalPages = record.totalPages;
        state.songs = record.songs;
        state.results.clear();
        for (const Song &song : state.songs)
            state.results.append(songToResultItem(song));
    }

    m_searchSource = qBound(0, settings.value(QStringLiteral("currentSearchSource"), 0).toInt(), 1);

    if (settings.contains(QStringLiteral("volume"))) {
        int vol = qBound(0, settings.value(QStringLiteral("volume")).toInt(), 100);
        m_audioOutput->setVolume(vol / 100.0f);
    }

    QByteArray deviceId = QByteArray::fromBase64(settings.value(QStringLiteral("selectedAudioDeviceId")).toString().toLatin1());
    if (!deviceId.isEmpty() && applyAudioDevice(deviceId)) {
        m_userSelectedDevice = true;
        m_selectedDeviceId = deviceId;
        refreshAudioDevices();
    }

    int currentIndex = -1;
    int playMode = 0;
    QVector<Song> queue = m_stateStore->loadQueue(&currentIndex, &playMode);
    m_playlist->restoreSongs(queue, currentIndex);
    m_playlist->setPlayMode(static_cast<PlaylistManager::PlayMode>(qBound(0, playMode, 2)));
    Song current = m_playlist->getCurrentSong();
    if (current.source == SearchSource::Bilibili ? !current.bvid.isEmpty() : current.id != -1)
        restoreCurrentSongInfo(current);
}

void MusicController::saveCurrentSearchState()
{
    if (!m_stateStore) return;
    const SearchState &state = activeSearchState();
    m_stateStore->saveSearchState(currentSearchSource(), state.keywords, state.page, state.totalPages, state.songs);
}

void MusicController::savePlaybackState()
{
    if (!m_stateStore) return;
    m_stateStore->saveQueue(m_playlist->songs(), m_playlist->getCurrentIndex(), static_cast<int>(m_playlist->getPlayMode()));
}

void MusicController::restoreCurrentSongInfo(const Song &song)
{
    m_currentSongName = song.name;
    m_currentArtist = song.artist;
    if (song.source == SearchSource::Bilibili) {
        m_currentBvid = song.bvid;
        m_currentPlayingSongId = -1;
        m_currentLyric = tr("Bilibili 视频 · 无歌词");
        if (!song.picUrl.isEmpty())
            m_albumArtUrl = QUrl(song.picUrl);
    } else {
        m_currentPlayingSongId = song.id;
        m_currentBvid.clear();
        m_currentLyric.clear();
    }
    emit currentSongChanged();
    emit lyricChanged();
    emit albumArtChanged();
    emit hasMediaChanged();
}

SearchSource MusicController::currentSearchSource() const
{
    return m_searchSource == 1 ? SearchSource::Bilibili : SearchSource::NetEase;
}

MusicController::SearchState &MusicController::activeSearchState()
{
    return m_searchStates[m_searchSource == 1 ? 1 : 0];
}

const MusicController::SearchState &MusicController::activeSearchState() const
{
    return m_searchStates[m_searchSource == 1 ? 1 : 0];
}

QVariantMap MusicController::songToResultItem(const Song &song)
{
    QVariantMap item;
    item["title"] = song.name;
    item["artist"] = song.artist;
    item["source"] = static_cast<int>(song.source);
    if (song.source == SearchSource::Bilibili)
        item["bvid"] = song.bvid;
    else
        item["id"] = song.id;
    return item;
}

void MusicController::setPlaybackLoading(bool loading)
{
    if (m_playbackLoading == loading) return;
    m_playbackLoading = loading;
    emit loadingChanged();
}

void MusicController::setSearchLoading(bool loading)
{
    if (m_searchLoading == loading) return;
    m_searchLoading = loading;
    emit searchLoadingChanged();
}

void MusicController::showStatus(const QString &message, bool isError)
{
    m_statusMessage = message;
    m_statusIsError = isError;
    emit statusMessageChanged();
}

void MusicController::cleanupTempAudio()
{
    if (m_tempCleanupConn)
        disconnect(m_tempCleanupConn);
    if (!m_tempAudioPath.isEmpty()) {
        QFile::remove(m_tempAudioPath);
        m_tempAudioPath.clear();
    }
}

void MusicController::refreshAudioDevices()
{
    m_audioDevices.clear();
    const auto devs = QMediaDevices::audioOutputs();
    QByteArray activeId = m_userSelectedDevice ? m_selectedDeviceId : m_audioOutput->device().id();
    m_currentAudioDeviceIndex = 0;
    for (int i = 0; i < devs.size(); ++i) {
        QVariantMap d;
        d["name"] = devs[i].description();
        d["id"] = devs[i].id();
        m_audioDevices.append(d);
        if (devs[i].id() == activeId)
            m_currentAudioDeviceIndex = i;
    }
    emit audioDevicesChanged();
}

bool MusicController::applyAudioDevice(const QByteArray &deviceId)
{
    const auto devs = QMediaDevices::audioOutputs();
    for (const auto &d : devs) {
        if (d.id() == deviceId) {
            m_audioOutput->setDevice(d);
            return true;
        }
    }
    return false;
}

void MusicController::parseLyrics(const QString &lyricText)
{
    m_lyricData.clear();
    QRegularExpression re(R"(\[(\d{2}):(\d{2})\.(\d{2,3})\](.*))");
    for (const QString &line : lyricText.split('\n')) {
        auto match = re.match(line);
        if (match.hasMatch()) {
            qint64 ms = match.captured(1).toLongLong() * 60000
                      + match.captured(2).toLongLong() * 1000
                      + (match.captured(3).length() == 2
                             ? match.captured(3).toLongLong() * 10
                             : match.captured(3).toLongLong());
            m_lyricData.insert(ms, match.captured(4));
        }
    }
}

void MusicController::updateCurrentLyric(qint64 positionMs)
{
    if (m_lyricData.isEmpty()) return;
    auto it = m_lyricData.upperBound(positionMs);
    if (it != m_lyricData.begin()) {
        --it;
        QString newLyric = it.value();
        if (newLyric != m_currentLyric) {
            m_currentLyric = newLyric;
            emit lyricChanged();
        }
    }
}

QVariantList MusicController::extractPaletteColors(const QImage &image, int count)
{
    QVariantList colors;
    if (image.isNull()) return colors;

    QImage small = image.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int w = small.width(), h = small.height();

    struct Region { int x1,y1,x2,y2; };
    QVector<Region> regions = {
        {0,0,w/2,h/2},{w/2,0,w,h/2},{w/4,h/4,w*3/4,h*3/4},{0,h/2,w/2,h},{w/2,h/2,w,h}
    };

    QVector<QColor> regionColors;
    for (auto &r : regions) {
        long rr=0,gg=0,bb=0; int cnt=0;
        for (int y=r.y1;y<r.y2&&y<h;++y)
            for (int x=r.x1;x<r.x2&&x<w;++x) {
                QColor c = small.pixelColor(x,y);
                rr+=c.red(); gg+=c.green(); bb+=c.blue(); cnt++;
            }
        if (cnt>0) regionColors.append(QColor(rr/cnt,gg/cnt,bb/cnt));
    }

    std::sort(regionColors.begin(),regionColors.end(),[](const QColor&a,const QColor&b){
        return (a.red()*0.299+a.green()*0.587+a.blue()*0.114) >
               (b.red()*0.299+b.green()*0.587+b.blue()*0.114);
    });

    QVector<QColor> picked;
    for (auto &c : regionColors) {
        bool sim=false;
        for (auto &e : picked)
            if (qAbs(c.red()-e.red())+qAbs(c.green()-e.green())+qAbs(c.blue()-e.blue())<80) { sim=true; break; }
        if (!sim) { picked.append(c); if (picked.size()>=count) break; }
    }
    while (picked.size()<count && !picked.isEmpty())
        picked.append(picked.first().darker(120+picked.size()*30));
    if (picked.isEmpty()) picked.append(QColor(51,51,51));

    for (auto &c : picked) colors.append(c.name());
    return colors;
}

// ============================================================
// Private playback helpers
// ============================================================

void MusicController::playSong(qint64 id)
{
    if (id <= 0) return;
    qCInfo(logPlayer) << "Play NetEase song id =" << id;
    setPlaybackLoading(true);

    // Reset state
    m_pendingCoverUrl = QUrl();
    m_currentBilibiliAudioUrl = QUrl();
    m_currentBilibiliAudioBvid.clear();
    m_bilibiliFallbackPending = false;
    m_bilibiliPages.clear();
    m_currentBilibiliPageIndex = 0;
    m_currentBilibiliCid = -1;
    cleanupTempAudio();
    m_lyricData.clear();
    m_albumArtUrl = QUrl();
    m_paletteColors.clear();
    emit bilipagesChanged();
    emit albumArtChanged();
    emit paletteColorsChanged();

    m_currentPlayingSongId = id;
    m_currentBvid.clear();
    m_pendingCoverSource = 0; // NetEase

    Song cur = m_playlist->getCurrentSong();
    if (cur.id == id) {
        m_currentSongName = cur.name;
        m_currentArtist = cur.artist;
    } else {
        m_currentSongName = tr("加载中...");
        m_currentArtist.clear();
    }
    m_currentLyric = tr("歌词加载中...");
    emit currentSongChanged();
    emit lyricChanged();
    emit hasMediaChanged();

    m_api->getSongUrl(id);
    m_api->getLyric(id);
    m_api->getSongDetail(id);
}

void MusicController::playBilibiliVideo(const QString &bvid)
{
    if (bvid.isEmpty()) return;
    qCInfo(logPlayer).noquote() << "Play Bilibili video bvid =" << bvid;
    setPlaybackLoading(true);

    m_pendingCoverUrl = QUrl();
    m_currentBilibiliAudioUrl = QUrl();
    m_currentBilibiliAudioBvid.clear();
    m_bilibiliFallbackPending = false;
    m_bilibiliPages.clear();
    m_currentBilibiliPageIndex = 0;
    m_currentBilibiliCid = -1;
    cleanupTempAudio();
    m_lyricData.clear();
    m_albumArtUrl = QUrl();
    m_paletteColors.clear();
    emit bilipagesChanged();
    emit albumArtChanged();
    emit paletteColorsChanged();

    m_currentBvid = bvid;
    m_currentPlayingSongId = -1;
    m_pendingCoverSource = 1; // Bilibili

    Song cur = m_playlist->getCurrentSong();
    if (cur.bvid == bvid) {
        m_currentSongName = cur.name;
        m_currentArtist = cur.artist;
    } else {
        m_currentSongName = tr("加载中...");
        m_currentArtist.clear();
    }
    m_currentLyric = tr("Bilibili 视频 · 无歌词");
    emit currentSongChanged();
    emit lyricChanged();
    emit hasMediaChanged();

    m_api->getBilibiliVideoInfo(bvid);
}

// ============================================================
// Slots — API callbacks
// ============================================================

void MusicController::onSearchFinished(const QJsonDocument &json, const QString &keywords, int limit, int offset)
{
    Q_UNUSED(limit)
    int page = offset / 15 + 1;
    SearchState &state = m_searchStates[0];
    if (m_searchSource != 0 || keywords != state.keywords || page != state.page) return;

    setSearchLoading(false);
    state.results.clear();
    state.songs.clear();

    QJsonObject root = json.object();
    int total = 0;
    if (root.contains("result")) {
        QJsonObject res = root["result"].toObject();
        total = res["songCount"].toInt();
        QJsonArray songs = res["songs"].toArray();
        if (songs.isEmpty() && state.page == 1) showStatus(tr("未找到相关歌曲。"));
        for (const QJsonValue &v : songs) {
            QJsonObject obj = v.toObject();
            QString name = obj["name"].toString();
            QString artist;
            if (!obj["artists"].toArray().isEmpty())
                artist = obj["artists"].toArray()[0].toObject()["name"].toString();
            qint64 id = obj["id"].toVariant().toLongLong();

            Song s; s.id=id; s.name=name; s.artist=artist; s.source=SearchSource::NetEase;
            state.songs.append(s);
            state.results.append(songToResultItem(s));
        }
    }
    state.totalPages = total > 0 ? (total + 14) / 15 : 0;
    saveCurrentSearchState();
    emit searchResultsChanged();
    emit pageChanged();
}

void MusicController::onBilibiliSearchFinished(const QJsonDocument &json, const QString &keywords, int page)
{
    SearchState &state = m_searchStates[1];
    if (m_searchSource != 1 || keywords != state.keywords || page != state.page) return;
    setSearchLoading(false);
    state.results.clear();
    state.songs.clear();

    QJsonObject root = json.object();
    if (root.value("code").toInt() != 0) {
        qCWarning(logPlayer) << "Bilibili search API error:" << root.value("message").toString();
        return;
    }
    QJsonObject data = root.value("data").toObject();
    int total = data.value("numResults").toInt();
    QJsonArray videos = data.value("result").toObject().value("video").toArray();
    if (videos.isEmpty() && state.page == 1) showStatus(tr("未找到相关视频。"));

    for (const QJsonValue &v : videos) {
        QJsonObject obj = v.toObject();
        QString bvid = obj["bvid"].toString();
        QString title = obj["title"].toString();
        title.remove(QRegularExpression("<[^>]*>"));
        QString author = obj["author"].toString();
        QString pic = obj["pic"].toString();
        if (!pic.startsWith("http")) pic = "https:" + pic;

        Song s; s.bvid=bvid; s.name=title; s.artist=author; s.picUrl=pic; s.source=SearchSource::Bilibili;
        state.songs.append(s);
        state.results.append(songToResultItem(s));
    }
    state.totalPages = total > 0 ? (total + 19) / 20 : 0;
    saveCurrentSearchState();
    emit searchResultsChanged();
    emit pageChanged();
}

void MusicController::onLyricFinished(const QJsonDocument &json, qint64 songId)
{
    if (songId != m_currentPlayingSongId) return;
    m_lyricData.clear();
    QJsonObject root = json.object();
    if (root.contains("lrc"))
        parseLyrics(root["lrc"].toObject()["lyric"].toString());
}

void MusicController::onSongDetailFinished(const QJsonDocument &json, qint64 songId)
{
    if (songId != m_currentPlayingSongId) return;
    QJsonObject root = json.object();
    if (root.contains("songs")) {
        QJsonArray arr = root["songs"].toArray();
        if (!arr.isEmpty()) {
            QString picUrl = arr[0].toObject()["album"].toObject()["picUrl"].toString() + "?param=300y300";
            m_pendingCoverUrl = QUrl(picUrl);
            m_pendingCoverSource = 0;
            m_api->downloadImage(m_pendingCoverUrl);
        }
    }
}

void MusicController::onImageDownloaded(const QByteArray &data, const QUrl &url)
{
    if (m_currentPlayingSongId == -1 || m_pendingCoverSource != 0 || url != m_pendingCoverUrl) return;
    QImage img;
    if (img.loadFromData(data)) {
        m_albumArtImage = img;
        // Save to temp file for QML Image source
        QString path = QDir::tempPath() + "/melody_cover.jpg";
        img.save(path, "JPEG");
        m_albumArtUrl = QUrl::fromLocalFile(path);
        m_albumArtUrl.setQuery("t=" + QString::number(QDateTime::currentMSecsSinceEpoch()));
        emit albumArtChanged();

        m_paletteColors = extractPaletteColors(img);
        emit paletteColorsChanged();
    }
}

void MusicController::onBilibiliImageDownloaded(const QByteArray &data, const QUrl &url)
{
    if (m_currentBvid.isEmpty() || m_pendingCoverSource != 1 || url != m_pendingCoverUrl) return;
    QImage img;
    if (img.loadFromData(data)) {
        m_albumArtImage = img;
        QString path = QDir::tempPath() + "/melody_cover.jpg";
        img.save(path, "JPEG");
        m_albumArtUrl = QUrl::fromLocalFile(path);
        m_albumArtUrl.setQuery("t=" + QString::number(QDateTime::currentMSecsSinceEpoch()));
        emit albumArtChanged();

        m_paletteColors = extractPaletteColors(img);
        emit paletteColorsChanged();
    }
}

void MusicController::onSongUrlReady(const QUrl &url, qint64 songId)
{
    if (songId != m_currentPlayingSongId) return;
    m_player->setSource(url);
    m_player->play();
}

void MusicController::onBilibiliVideoInfoFinished(const QJsonDocument &json, const QString &requestBvid)
{
    if (requestBvid != m_currentBvid) return;
    QJsonObject root = json.object();
    if (root.value("code").toInt() != 0) {
        qCWarning(logPlayer) << "Bilibili videoInfo API error:" << root.value("message").toString();
        setPlaybackLoading(false);
        return;
    }
    QJsonObject data = root.value("data").toObject();
    QString bvid = data.value("bvid").toString();
    qint64 cid = data.value("cid").toVariant().toLongLong();
    QString pic = data.value("pic").toString();

    m_bilibiliPages.clear();
    QJsonArray pages = data.value("pages").toArray();
    if (pages.size() > 1) {
        for (const QJsonValue &v : pages) {
            QJsonObject p = v.toObject();
            QVariantMap pm;
            pm["cid"] = p.value("cid").toVariant().toLongLong();
            pm["label"] = p.value("part").toString().isEmpty()
                ? QString("P%1").arg(p.value("page").toInt())
                : QString("P%1 %2").arg(p.value("page").toInt()).arg(p.value("part").toString());
            m_bilibiliPages.append(pm);
        }
        cid = m_bilibiliPages.first().toMap().value("cid").toLongLong();
        m_currentBilibiliPageIndex = 0;
    }
    m_currentBilibiliCid = cid;
    emit bilipagesChanged();

    if (!pic.isEmpty()) {
        if (!pic.startsWith("http")) pic = "https:" + pic;
        if (pic.contains("hdslb.com")) {
            pic += "@300w_300h.jpg";
        }
        m_pendingCoverUrl = QUrl(pic);
        m_pendingCoverSource = 1;
        m_api->downloadBilibiliImage(m_pendingCoverUrl);
    }
    m_api->getBilibiliAudioUrl(bvid, cid);
}

void MusicController::onBilibiliAudioUrlReady(const QUrl &url, const QString &bvid, qint64 cid)
{
    if (bvid != m_currentBvid || (m_currentBilibiliCid != -1 && cid != m_currentBilibiliCid)) return;
    // 优先尝试直连流式播放(mcdn 等 CDN 可秒播)。upos-sz-* CDN 要求 Referer,QMediaPlayer
    // 直连会 403 —— 由 onMediaPlayerError 捕获后回退到带 Referer 的下载模式。
    m_bilibiliFallbackPending = false;
    m_currentBilibiliAudioUrl = url;
    m_currentBilibiliAudioBvid = bvid;
    m_player->setSource(url);
    m_player->play();
}

void MusicController::onBilibiliAudioFileReady(const QString &filePath, const QString &bvid)
{
    // 过期下载：用户已切到别的视频，丢弃此文件并立即删除，避免磁盘泄漏
    if (bvid != m_currentBvid) {
        QFile::remove(filePath);
        return;
    }
    cleanupTempAudio();
    setPlaybackLoading(false);
    m_bilibiliFallbackPending = false;
    m_tempAudioPath = filePath;
    m_player->setSource(QUrl::fromLocalFile(filePath));
    m_player->play();
    m_tempCleanupConn = connect(m_player, &QMediaPlayer::playbackStateChanged, this,
        [this, filePath](QMediaPlayer::PlaybackState s) {
            if (s == QMediaPlayer::StoppedState && m_tempAudioPath == filePath)
                cleanupTempAudio();
        });
}

void MusicController::onApiError(const QString &errorString)
{
    if (errorString.contains("mp3") && m_currentPlayingSongId != -1) {
        qCInfo(logPlayer) << "SongUrl failed, falling back to NetEase outer url for songId" << m_currentPlayingSongId;
        QString fallback = QString("https://music.163.com/song/media/outer/url?id=%1.mp3").arg(m_currentPlayingSongId);
        m_player->setSource(QUrl(fallback));
        m_player->play();
        return;
    }
    qCWarning(logPlayer).noquote() << "API error:" << errorString;
    setSearchLoading(false);
    setPlaybackLoading(false);
}

// ============================================================
// Slots — Media player
// ============================================================

void MusicController::onPositionChanged(qint64 pos)
{
    emit positionChanged();
    updateCurrentLyric(pos);
}

void MusicController::onDurationChanged(qint64 /*dur*/)
{
    emit durationChanged();
    emit hasMediaChanged();
}

void MusicController::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    qCDebug(logPlayer) << "PlaybackState:" << state;
    if (state == QMediaPlayer::PlayingState && m_playbackLoading)
        setPlaybackLoading(false);
    emit playingChanged();
}

void MusicController::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    // BufferingMedia/BufferedMedia 在播放过程中每隔几百毫秒来回跳,纯噪声,不打日志。
    if (status != QMediaPlayer::BufferingMedia && status != QMediaPlayer::BufferedMedia)
        qCDebug(logPlayer) << "MediaStatus:" << status;
    if (status == QMediaPlayer::EndOfMedia) {
        qCInfo(logPlayer) << "EndOfMedia -> playNext";
        playNext();
        emit hasMediaChanged();
    }
}

void MusicController::onMediaPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    // 直连 Bilibili 流遇到 403(upos CDN 要求 Referer):预期内,降级为 INFO 并回退到下载模式。
    if (error == QMediaPlayer::ResourceError &&
        !m_currentBilibiliAudioUrl.isEmpty() &&
        !m_currentBvid.isEmpty() &&
        m_currentBilibiliAudioBvid == m_currentBvid)
    {
        qCInfo(logPlayer).noquote() << "Bilibili direct stream blocked (" << errorString
                                    << "), fallback to download mode";
        m_bilibiliFallbackPending = true;   // 抑制回退过程中 GStreamer 拆除直连管道的残留错误
        m_player->stop();
        m_api->downloadBilibiliAudio(m_currentBilibiliAudioUrl, m_currentBvid);
        m_currentBilibiliAudioUrl.clear();
        m_currentBilibiliAudioBvid.clear();
        return;
    }

    // 回退下载期间 GStreamer 拆管产生的残留错误(Internal data stream error / 流中没有足够数据),
    // 属于噪声,直接忽略。
    if (m_bilibiliFallbackPending && error == QMediaPlayer::ResourceError)
        return;

    qCWarning(logPlayer) << "MediaPlayer error:" << error
                         << "|" << (errorString.isEmpty() ? QStringLiteral("(no detail)") : errorString);
    setPlaybackLoading(false);
}

void MusicController::onAudioOutputsChanged()
{
    qCInfo(logPlayer) << "System audio outputs changed;";
    if (m_userSelectedDevice && applyAudioDevice(m_selectedDeviceId)) {
        refreshAudioDevices();
        return;
    }
    m_userSelectedDevice = false;
    m_selectedDeviceId.clear();
    QAudioDevice newDefault = QMediaDevices::defaultAudioOutput();
    if (!newDefault.isNull())
        m_audioOutput->setDevice(newDefault);
    refreshAudioDevices();
}
