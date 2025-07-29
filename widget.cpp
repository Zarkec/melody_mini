#include "widget.h"
#include "apimanager.h"
#include "playlistmanager.h" // 集成播放列表
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QAudioOutput>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QRegularExpression>
#include <QPixmap>
#include <QFont>
#include <QMenu>
#include <QWidgetAction>

Widget::Widget(QWidget *parent)
    : QWidget(parent), currentDuration(0)
{
    // --- 新增：播放列表管理器初始化 ---
    playlistManager = new PlaylistManager(this);
    // --- UI 控件初始化 ---
    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("输入歌名或歌手...");
    searchButton = new QPushButton("搜索");
    resultList = new QListWidget;
    playPauseButton = new QPushButton("▶");
    prevButton = new QPushButton("⏮");
    nextButton = new QPushButton("⏭");
    playModeButton = new QPushButton("顺序"); // 初始化播放模式按钮
    playModeButton->setFixedWidth(80);
    progressSlider = new QSlider(Qt::Horizontal);
    timeLabel = new QLabel("00:00 / 00:00");

    // --- 音量控制 ---
    volumeButton = new QPushButton("🔊"); // 使用Emoji作为图标
    volumeButton->setFixedSize(35, 35);
    volumeButton->setFlat(true); // 使按钮看起来更像一个图标

    volumeSlider = new QSlider(Qt::Vertical); // 设置为垂直
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);
    volumeSlider->setFixedHeight(100); // 设置高度

    volumeMenu = new QMenu(this);
    volumeAction = new QWidgetAction(this);
    volumeAction->setDefaultWidget(volumeSlider);
    volumeMenu->addAction(volumeAction);

    // 播放详情页
    playerPage = new QWidget;
    albumArtLabel = new QLabel;
    albumArtLabel->setFixedSize(300, 300);
    albumArtLabel->setScaledContents(true);
    lyricLabel = new QLabel("欢迎使用 Melody");
    lyricLabel->setAlignment(Qt::AlignCenter);
    lyricLabel->setWordWrap(true);
    QFont lyricFont = lyricLabel->font();
    lyricFont.setPointSize(14);
    lyricLabel->setFont(lyricFont);

    QVBoxLayout *playerPageLayout = new QVBoxLayout(playerPage);
    playerPageLayout->addWidget(albumArtLabel, 0, Qt::AlignCenter);
    playerPageLayout->addWidget(lyricLabel);

    // 主堆叠窗口
    mainStackedWidget = new QStackedWidget;
    mainStackedWidget->addWidget(resultList);
    mainStackedWidget->addWidget(playerPage);
    mainStackedWidget->setCurrentWidget(playerPage); // 默认显示播放页

    // --- 布局设置 ---
    topLayout = new QHBoxLayout;
    topLayout->addWidget(searchInput);
    topLayout->addWidget(searchButton);

    bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(prevButton);
    bottomLayout->addWidget(playPauseButton);
    bottomLayout->addWidget(nextButton);
    bottomLayout->addWidget(playModeButton); // 添加到布局
    bottomLayout->addWidget(progressSlider);
    bottomLayout->addWidget(timeLabel);
    bottomLayout->addWidget(volumeButton); // 添加新的音量按钮

    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(mainStackedWidget);
    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);
    setWindowTitle("Melody");
    resize(400, 400);

    // --- 样式表设置 ---
    QString styleSheet = R"(
        QWidget {
            background-color: #191919;
            color: #E0E0E0;
            font-family: 'Microsoft YaHei';
        }
        QLineEdit {
            background-color: #2D2D2D;
            border: 1px solid #454545;
            border-radius: 5px;
            padding: 5px;
            color: #E0E0E0;
        }
        QPushButton {
            background-color: #2D2D2D;
            border: 1px solid #454545;
            border-radius: 5px;
            padding: 5px 10px;
        }
        QPushButton:hover {
            background-color: #3D3D3D;
        }
        QPushButton:pressed {
            background-color: #1AD6C9;
            color: #191919;
        }
        QListWidget {
            background-color: #2D2D2D;
            border: 1px solid #454545;
            border-radius: 5px;
        }
        QListWidget::item {
            padding: 10px;
        }
        QListWidget::item:hover {
            background-color: #3D3D3D;
        }
        QListWidget::item:selected {
            background-color: #1AD6C9;
            color: #191919;
        }
        QSlider::groove:horizontal {
            border: 1px solid #454545;
            height: 4px;
            background: #3D3D3D;
            margin: 2px 0;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #1AD6C9;
            border: 1px solid #1AD6C9;
            width: 12px;
            margin: -4px 0;
            border-radius: 6px;
        }
        QSlider::sub-page:horizontal {
            background: #1AD6C9;
            border: 1px solid #454545;
            height: 4px;
            border-radius: 2px;
        }
        QSlider::groove:vertical {
            border: 1px solid #454545;
            width: 4px;
            background: #3D3D3D;
            margin: 0 2px;
            border-radius: 2px;
        }
        QSlider::handle:vertical {
            background: #1AD6C9;
            border: 1px solid #1AD6C9;
            height: 12px;
            margin: 0 -4px;
            border-radius: 6px;
        }
        QSlider::add-page:vertical {
            background: #1AD6C9;
            border: 1px solid #454545;
            width: 4px;
            border-radius: 2px;
        }
        QMenu {
            background-color: #2D2D2D;
            border: 1px solid #454545;
        }
    )";
    this->setStyleSheet(styleSheet);

    // --- 后端对象初始化 ---
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.5);
    apiManager = new ApiManager(this);

    // --- 信号与槽连接 ---
    connect(searchButton, &QPushButton::clicked, this, &Widget::onSearchButtonClicked);
    connect(searchInput, &QLineEdit::returnPressed, this, &Widget::onSearchButtonClicked);
    connect(apiManager, &ApiManager::searchFinished, this, &Widget::onSearchFinished);
    connect(apiManager, &ApiManager::lyricFinished, this, &Widget::onLyricFinished);
    connect(apiManager, &ApiManager::songDetailFinished, this, &Widget::onSongDetailFinished);
    connect(apiManager, &ApiManager::imageDownloaded, this, &Widget::onImageDownloaded);
    connect(apiManager, &ApiManager::songUrlReady, this, &Widget::onSongUrlReady);
    connect(apiManager, &ApiManager::error, this, &Widget::onApiError);
    connect(resultList, &QListWidget::itemDoubleClicked, this, &Widget::onResultItemDoubleClicked);
    connect(playPauseButton, &QPushButton::clicked, this, &Widget::onPlayPauseButtonClicked);
    connect(volumeButton, &QPushButton::clicked, this, &Widget::onVolumeButtonClicked); // 连接音量按钮
    connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        audioOutput->setVolume(value / 100.0);
    });
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &Widget::updatePosition);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &Widget::updateDuration);
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &Widget::updateState);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &Widget::onMediaStatusChanged); // 监听播放结束
    connect(progressSlider, &QSlider::sliderMoved, this, &Widget::setPosition);

    // 新增：连接上一曲/下一曲/播放模式按钮
    connect(prevButton, &QPushButton::clicked, this, &Widget::playPreviousSong);
    connect(nextButton, &QPushButton::clicked, this, &Widget::playNextSong);
    connect(playModeButton, &QPushButton::clicked, this, &Widget::changePlayMode);
}

Widget::~Widget() {}

void Widget::onSearchButtonClicked()
{
    QString keywords = searchInput->text();
    if (!keywords.isEmpty()) {
        mainStackedWidget->setCurrentWidget(resultList);
        resultList->clear();
        searchButton->setEnabled(false);
        searchButton->setText("搜索中...");
        apiManager->searchSongs(keywords);
    }
}

void Widget::onSearchFinished(const QJsonDocument &json)
{
    searchButton->setEnabled(true);
    searchButton->setText("搜索");

    searchResultSongs.clear(); // 清空旧的搜索结果

    QJsonObject rootObj = json.object();
    if (rootObj.contains("result")) {
        QJsonObject resultObj = rootObj["result"].toObject();
        if (resultObj.contains("songs")) {
            QJsonArray songsArray = resultObj["songs"].toArray();
            for (const QJsonValue &value : songsArray) {
                QJsonObject songObj = value.toObject();
                
                QString songName = songObj["name"].toString();
                QString artistName;
                if (!songObj["artists"].toArray().isEmpty()) {
                    artistName = songObj["artists"].toArray()[0].toObject()["name"].toString();
                }
                qint64 songId = songObj["id"].toVariant().toLongLong();

                // 添加到UI列表
                QListWidgetItem *item = new QListWidgetItem(QString("%1 - %2").arg(songName, artistName));
                item->setData(Qt::UserRole, songId);
                resultList->addItem(item);

                // 添加到歌曲结构体列表
                searchResultSongs.append({songId, songName, artistName});
            }
            // 将完整的搜索结果列表交给播放列表管理器
            playlistManager->addSongs(searchResultSongs);
        }
    }
}

void Widget::onVolumeButtonClicked()
{
    // 在按钮上方居中显示菜单
    QPoint pos = volumeButton->mapToGlobal(QPoint(0, 0));
    // 计算水平居中位置
    pos.setX(pos.x() - (volumeMenu->sizeHint().width() - volumeButton->width()) / 2);
    // 计算垂直位置，使菜单显示在按钮上方
    pos.setY(pos.y() - volumeMenu->sizeHint().height());
    volumeMenu->exec(pos);
}

void Widget::onLyricFinished(const QJsonDocument &json)
{
    lyricData.clear();
    QJsonObject rootObj = json.object();
    if (rootObj.contains("lrc")) {
        QString lyricText = rootObj["lrc"].toObject()["lyric"].toString();
        parseLyrics(lyricText);
    }
}

void Widget::onSongDetailFinished(const QJsonDocument &json)
{
    QJsonObject rootObj = json.object();
    if (rootObj.contains("songs")) {
        QJsonArray songsArray = rootObj["songs"].toArray();
        if (!songsArray.isEmpty()) {
            QJsonObject songObj = songsArray[0].toObject();
            if (songObj.contains("album")) {
                QString imageUrl = songObj["album"].toObject()["picUrl"].toString() + "?param=800y800";
                apiManager->downloadImage(QUrl(imageUrl));
            }
        }
    }
}

void Widget::onImageDownloaded(const QByteArray &data)
{
    QPixmap pixmap;
    pixmap.loadFromData(data);
    albumArtLabel->setPixmap(pixmap);
}

void Widget::onSongUrlReady(const QUrl &url)
{
    mediaPlayer->setSource(url);
    mediaPlayer->play();
}

void Widget::onApiError(const QString &errorString)
{
    searchButton->setEnabled(true);
    searchButton->setText("搜索");
    QMessageBox::critical(this, "网络错误", errorString);
}

void Widget::onResultItemDoubleClicked(QListWidgetItem *item)
{
    int index = resultList->row(item);
    playlistManager->setCurrentIndex(index);
    
    Song currentSong = playlistManager->getCurrentSong();
    if (currentSong.id != -1) {
        playSong(currentSong.id);
    }
}

void Widget::onPlayPauseButtonClicked()
{
    if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        mediaPlayer->pause();
    } else {
        mediaPlayer->play();
    }
}

void Widget::updatePosition(qint64 position)
{
    progressSlider->setValue(position);
    
    // 更新时间显示
    qint64 totalSeconds = position / 1000;
    qint64 minutes = totalSeconds / 60;
    qint64 seconds = totalSeconds % 60;
    qint64 totalDurationSeconds = currentDuration / 1000;
    qint64 totalMinutes = totalDurationSeconds / 60;
    qint64 totalSecondsFormatted = totalDurationSeconds % 60;
    timeLabel->setText(QString("%1:%2 / %3:%4").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')).arg(totalMinutes, 2, 10, QChar('0')).arg(totalSecondsFormatted, 2, 10, QChar('0')));

    // 更新歌词
    if (!lyricData.isEmpty()) {
        auto it = lyricData.upperBound(position);
        if (it != lyricData.begin()) {
            --it;
            lyricLabel->setText(it.value());
        }
    }
}

void Widget::updateDuration(qint64 duration)
{
    currentDuration = duration;
    progressSlider->setRange(0, duration);
}

void Widget::updateState(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::PlayingState) {
        playPauseButton->setText("⏸");
    } else {
        playPauseButton->setText("▶");
    }
}

void Widget::setPosition(int position)
{
    mediaPlayer->setPosition(position);
}

// --- 新增的私有和槽函数实现 ---

void Widget::playSong(qint64 id)
{
    if (id <= 0) return;

    // 请求播放链接
    apiManager->getSongUrl(id);

    // 获取歌词和详情
    apiManager->getLyric(id);
    apiManager->getSongDetail(id);

    // 切换到播放详情页
    mainStackedWidget->setCurrentWidget(playerPage);
}

void Widget::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    // 当歌曲播放结束时，自动播放下一首
    if (status == QMediaPlayer::EndOfMedia) {
        playNextSong();
    }
}

void Widget::playNextSong()
{
    if (playlistManager->isEmpty()) return;

    Song nextSong = playlistManager->getNextSong();
    if (nextSong.id != -1) {
        playSong(nextSong.id);
    }
}

void Widget::playPreviousSong()
{
    if (playlistManager->isEmpty()) return;

    Song prevSong = playlistManager->getPreviousSong();
    if (prevSong.id != -1) {
        playSong(prevSong.id);
    }
}

void Widget::changePlayMode()
{
    PlaylistManager::PlayMode currentMode = playlistManager->getPlayMode();
    // 循环切换模式
    int nextModeIndex = (static_cast<int>(currentMode) + 1) % 3;
    PlaylistManager::PlayMode nextMode = static_cast<PlaylistManager::PlayMode>(nextModeIndex);
    playlistManager->setPlayMode(nextMode);

    // 更新UI图标和提示
    switch(nextMode) {
        case PlaylistManager::Sequential:
            playModeButton->setText("顺序播放");
            break;
        case PlaylistManager::LoopOne:
            playModeButton->setText("单曲循环");
            break;
        case PlaylistManager::Random:
            playModeButton->setText("随机播放");
            break;
    }
}

void Widget::parseLyrics(const QString &lyricText)
{
    QRegularExpression re("\\[(\\d{2}):(\\d{2})\\.(\\d{2,3})\\](.*)");
    for (const QString &line : lyricText.split('\n')) {
        QRegularExpressionMatch match = re.match(line);
        if (match.hasMatch()) {
            qint64 minutes = match.captured(1).toLongLong();
            qint64 seconds = match.captured(2).toLongLong();
            qint64 milliseconds = match.captured(3).toLongLong();
            if (match.captured(3).length() == 2) { // 兼容xx.xx格式
                milliseconds *= 10;
            }
            qint64 time = minutes * 60 * 1000 + seconds * 1000 + milliseconds;
            QString text = match.captured(4);
            lyricData.insert(time, text);
        }
    }
}
