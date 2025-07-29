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
    this->setObjectName("mainWidget");
    // --- 新增：播放列表管理器初始化 ---
    playlistManager = new PlaylistManager(this);

    // --- 业务逻辑变量初始化 ---
    currentPage = 1;
    currentPlayingSongId = -1;

    // --- 动态背景初始化 ---
    currentBackgroundColor = QColor(51, 51, 51);
    backgroundAnimation = new QPropertyAnimation(this, "widgetBackgroundColor", this);
    backgroundAnimation->setDuration(800);
    backgroundAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    // --- UI 控件初始化 ---
    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("输入歌名或歌手...");
    searchButton = new QPushButton("搜索");
    backButton = new QPushButton("返回");
    backButton->setVisible(false); // 默认隐藏
    resultList = new QListWidget;
    playPauseButton = new QPushButton("▶");
    prevButton = new QPushButton("⏮");
    nextButton = new QPushButton("⏭");
    playModeButton = new QPushButton("顺序"); // 初始化播放模式按钮
    progressSlider = new QSlider(Qt::Horizontal);
    timeLabel = new QLabel("00:00 / 00:00");

    // --- 分页控件 ---
    paginationWidget = new QWidget;
    prevPageButton = new QPushButton("< 上一页");
    nextPageButton = new QPushButton("下一页 >");
    pageLabel = new QLabel("第 1 页");
    prevPageButton->setEnabled(false); // 初始时禁用
    nextPageButton->setEnabled(false); // 初始时禁用

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
    songNameLabel = new QLabel("歌曲名称"); // 初始化
    songNameLabel->setAlignment(Qt::AlignCenter);
    QFont songNameFont = songNameLabel->font();
    songNameFont.setPointSize(16);
    songNameFont.setBold(true);
    songNameLabel->setFont(songNameFont);

    albumArtLabel = new QLabel;
    albumArtLabel->setScaledContents(true);
    lyricLabel = new QLabel("欢迎使用 Melody");
    lyricLabel->setAlignment(Qt::AlignCenter);
    lyricLabel->setWordWrap(true);
    QFont lyricFont = lyricLabel->font();
    lyricFont.setPointSize(14);
    lyricLabel->setFont(lyricFont);

    QVBoxLayout *playerPageLayout = new QVBoxLayout(playerPage);
    playerPageLayout->addWidget(songNameLabel); // 添加到布局
    playerPageLayout->addWidget(albumArtLabel, 0, Qt::AlignCenter);
    playerPageLayout->addWidget(lyricLabel);

    // 主堆叠窗口
    mainStackedWidget = new QStackedWidget;
    mainStackedWidget->addWidget(resultList);
    mainStackedWidget->addWidget(playerPage);
    mainStackedWidget->setCurrentWidget(resultList); // 默认显示搜索页

    // --- 布局设置 ---
    topLayout = new QHBoxLayout;
    topLayout->addWidget(backButton);
    topLayout->addWidget(searchInput);
    topLayout->addWidget(searchButton);

    paginationLayout = new QHBoxLayout(paginationWidget);
    paginationLayout->addStretch();
    paginationLayout->addWidget(prevPageButton);
    paginationLayout->addWidget(pageLabel);
    paginationLayout->addWidget(nextPageButton);
    paginationLayout->addStretch();
    paginationWidget->setLayout(paginationLayout);

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
    mainLayout->addWidget(paginationWidget); // 添加分页控件容器
    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);
    setWindowTitle("Melody");
    resize(400, 400);

    // --- 样式表设置 ---
    setWidgetStyle(currentBackgroundColor);

    // --- 后端对象初始化 ---
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.5);
    apiManager = new ApiManager(this);

    // --- 信号与槽连接 ---
    connect(mainStackedWidget, &QStackedWidget::currentChanged, this, &Widget::onMainStackCurrentChanged);
    connect(backButton, &QPushButton::clicked, this, &Widget::onBackButtonClicked);
    connect(prevPageButton, &QPushButton::clicked, this, &Widget::onPrevPageButtonClicked);
    connect(nextPageButton, &QPushButton::clicked, this, &Widget::onNextPageButtonClicked);
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
    currentSearchKeywords = searchInput->text();
    if (!currentSearchKeywords.isEmpty()) {
        currentPage = 1; // 每次新搜索都重置为第一页
        mainStackedWidget->setCurrentWidget(resultList);
        searchButton->setEnabled(false);
        searchButton->setText("搜索中...");
        // 调用带分页参数的搜索
        apiManager->searchSongs(currentSearchKeywords, 15, (currentPage - 1) * 15);
    }
}

void Widget::onSearchFinished(const QJsonDocument &json)
{
    searchButton->setEnabled(true);
    searchButton->setText("搜索");
    resultList->clear(); // 清空列表

    searchResultSongs.clear(); // 清空旧的歌曲数据

    QJsonObject rootObj = json.object();
    int totalSongCount = 0;
    if (rootObj.contains("result")) {
        QJsonObject resultObj = rootObj["result"].toObject();
        if (resultObj.contains("songCount")) {
            totalSongCount = resultObj["songCount"].toInt();
        }
        if (resultObj.contains("songs")) {
            QJsonArray songsArray = resultObj["songs"].toArray();
            if (songsArray.isEmpty() && currentPage == 1) {
                QMessageBox::information(this, "无结果", "未找到相关歌曲。");
            }
            for (const QJsonValue &value : songsArray) {
                QJsonObject songObj = value.toObject();
                
                QString songName = songObj["name"].toString();
                QString artistName;
                if (!songObj["artists"].toArray().isEmpty()) {
                    artistName = songObj["artists"].toArray()[0].toObject()["name"].toString();
                }
                qint64 songId = songObj["id"].toVariant().toLongLong();

                QListWidgetItem *item = new QListWidgetItem(QString("%1 - %2").arg(songName, artistName));
                item->setData(Qt::UserRole, songId);
                resultList->addItem(item);

                searchResultSongs.append({songId, songName, artistName});
            }
            playlistManager->addSongs(searchResultSongs);
        }
    }

    // 更新分页控件状态
    int totalPages = (totalSongCount > 0) ? (totalSongCount + 14) / 15 : 0;
    pageLabel->setText(QString("第 %1 / %2 页").arg(totalPages > 0 ? currentPage : 0).arg(totalPages));
    prevPageButton->setEnabled(currentPage > 1);
    nextPageButton->setEnabled(currentPage < totalPages);
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
    if (pixmap.loadFromData(data)) {
        originalAlbumArt = pixmap;
        int size = qMin(this->width(), this->height()) * 0.6;
        albumArtLabel->setPixmap(originalAlbumArt.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        
        QColor dominantColor = extractDominantColor(pixmap);
        updateBackgroundColor(dominantColor);
    }
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

void Widget::onBackButtonClicked()
{
    mainStackedWidget->setCurrentWidget(resultList);
}

void Widget::onResultItemDoubleClicked(QListWidgetItem *item)
{
    qint64 clickedSongId = item->data(Qt::UserRole).toLongLong();
    int index = resultList->row(item);
    Song clickedSong = searchResultSongs.at(index);

    // 检查点击的歌曲是否就是当前正在播放的歌曲
    if (clickedSongId == currentPlayingSongId && mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        // 如果是，并且播放器不是停止状态，则只切换回播放界面
        songNameLabel->setText(clickedSong.name); // 确保歌曲名称正确显示
        mainStackedWidget->setCurrentWidget(playerPage);
    } else {
        // 否则，按正常流程播放新歌曲
        playlistManager->setCurrentIndex(index);
        
        Song currentSong = playlistManager->getCurrentSong();
        if (currentSong.id != -1) {
            playSong(currentSong.id);
        }
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

    currentPlayingSongId = id; // 更新当前播放的歌曲ID

    // 从播放列表获取当前歌曲信息
    Song currentSong = playlistManager->getCurrentSong();
    if (currentSong.id == id) {
        songNameLabel->setText(currentSong.name);
    } else {
        songNameLabel->setText("加载中...");
    }


    // 重置UI
    originalAlbumArt = QPixmap();
    albumArtLabel->setPixmap(QPixmap());
    lyricLabel->setText("歌词加载中...");
    updateBackgroundColor(QColor(51, 51, 51));

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
        currentPlayingSongId = -1; // 播放结束，重置ID
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
            playModeButton->setText("顺序");
            break;
        case PlaylistManager::LoopOne:
            playModeButton->setText("单曲");
            break;
        case PlaylistManager::Random:
            playModeButton->setText("随机");
            break;
    }
}

void Widget::onPrevPageButtonClicked()
{
    if (currentPage > 1) {
        currentPage--;
        searchButton->setEnabled(false);
        searchButton->setText("加载中...");
        apiManager->searchSongs(currentSearchKeywords, 15, (currentPage - 1) * 15);
    }
}

void Widget::onNextPageButtonClicked()
{
    // 这里的总页数判断依赖于 onSearchFinished 的结果
    currentPage++;
    searchButton->setEnabled(false);
    searchButton->setText("加载中...");
    apiManager->searchSongs(currentSearchKeywords, 15, (currentPage - 1) * 15);
}

void Widget::onMainStackCurrentChanged(int index)
{
    // index 0: resultList, index 1: playerPage
    paginationWidget->setVisible(index == 0);
    backButton->setVisible(index == 1);
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

// --- 动态背景 ---

QColor Widget::getWidgetBackgroundColor() const
{
    return currentBackgroundColor;
}

QColor Widget::extractDominantColor(const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        return QColor(51, 51, 51); // 返回默认颜色
    }
    // 将图片缩放到1x1像素来获取平均颜色
    QImage image = pixmap.toImage().scaled(1, 1, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return image.pixelColor(0, 0);
}

bool Widget::isColorDark(const QColor &color) const
{
    // 使用亮度公式判断颜色深浅
    return (0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue()) < 128;
}

void Widget::setWidgetStyle(const QColor &color)
{
    currentBackgroundColor = color; // 更新当前颜色

    QString foregroundColor = isColorDark(color) ? "#E0E0E0" : "#212121";
    QString darkerColor = color.darker(150).name();

    // 重建样式表
    QString styleSheet = QString(R"(
        QWidget {
            background-color: transparent; 
            color: %1;
            font-family: 'Microsoft YaHei';
        }
        QLineEdit {
            background-color: rgba(0, 0, 0, 0.2);
            border: none;
            border-radius: 5px;
            padding: 5px;
            color: %1;
        }
        QPushButton {
            background-color: rgba(0, 0, 0, 0.2);
            border: none;
            border-radius: 5px;
            padding: 5px 10px;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.1);
        }
        QPushButton:pressed {
            background-color: %1;
            color: %2;
        }
        QListWidget {
            background-color: rgba(0, 0, 0, 0.2);
            border: none;
            border-radius: 5px;
        }
        QListWidget::item {
            padding: 10px;
            background-color: transparent;
        }
        QListWidget::item:hover {
            background-color: rgba(255, 255, 255, 0.1);
        }
        QListWidget::item:selected {
            background-color: %1;
            color: %2;
        }
        QSlider::groove:horizontal {
            border: none;
            height: 4px;
            background: #3D3D3D;
            margin: 2px 0;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: %1;
            border: none;
            width: 12px;
            margin: -4px 0;
            border-radius: 6px;
        }
        QSlider::sub-page:horizontal {
            background: %1;
            border: none;
            height: 4px;
            border-radius: 2px;
        }
        QSlider::groove:vertical {
            border: none;
            width: 4px;
            background: #3D3D3D;
            margin: 0 2px;
            border-radius: 2px;
        }
        QSlider::handle:vertical {
            background: %1;
            border: none;
            height: 12px;
            margin: 0 -4px;
            border-radius: 6px;
        }
        QSlider::add-page:vertical {
            background: %1;
            border: none;
            width: 4px;
            border-radius: 2px;
        }
        QMenu {
            background-color: %3;
            border: none;
        }
    )").arg(foregroundColor, color.name(), darkerColor);

    QString mainWidgetStyle = QString(
        "QWidget#mainWidget { background-color: qradialgradient(cx: 0.5, cy: 0.5, radius: 1.2, fx: 0.5, fy: 0.5, stop: 0 %1, stop: 1 %2); }"
    ).arg(color.name(), darkerColor);
    
    this->setStyleSheet(styleSheet + mainWidgetStyle);
}

void Widget::updateBackgroundColor(const QColor &newColor)
{
    if (backgroundAnimation->state() == QAbstractAnimation::Running) {
        backgroundAnimation->stop();
    }
    backgroundAnimation->setStartValue(currentBackgroundColor);
    backgroundAnimation->setEndValue(newColor);
    backgroundAnimation->start();
}

void Widget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!originalAlbumArt.isNull())
    {
        int size = qMin(this->width(), this->height()) * 0.6;
        albumArtLabel->setPixmap(originalAlbumArt.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
