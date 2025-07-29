#include "widget.h"
#include "apimanager.h"
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

Widget::Widget(QWidget *parent)
    : QWidget(parent), currentDuration(0)
{
    // --- UI 控件初始化 ---
    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("输入歌名或歌手...");
    searchButton = new QPushButton("搜索");
    resultList = new QListWidget;
    playPauseButton = new QPushButton("▶");
    prevButton = new QPushButton("⏮");
    nextButton = new QPushButton("⏭");
    progressSlider = new QSlider(Qt::Horizontal);
    timeLabel = new QLabel("00:00 / 00:00");
    volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);
    volumeSlider->setFixedWidth(100);

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
    bottomLayout->addWidget(progressSlider);
    bottomLayout->addWidget(timeLabel);
    bottomLayout->addWidget(new QLabel("音量:"));
    bottomLayout->addWidget(volumeSlider);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(mainStackedWidget);
    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);
    setWindowTitle("Melody");
    resize(800, 600);

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
    connect(apiManager, &ApiManager::searchFinished, this, &Widget::onSearchFinished);
    connect(apiManager, &ApiManager::lyricFinished, this, &Widget::onLyricFinished);
    connect(apiManager, &ApiManager::songDetailFinished, this, &Widget::onSongDetailFinished);
    connect(apiManager, &ApiManager::imageDownloaded, this, &Widget::onImageDownloaded);
    connect(apiManager, &ApiManager::error, this, &Widget::onApiError);
    connect(resultList, &QListWidget::itemDoubleClicked, this, &Widget::onResultItemDoubleClicked);
    connect(playPauseButton, &QPushButton::clicked, this, &Widget::onPlayPauseButtonClicked);
    connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        audioOutput->setVolume(value / 100.0);
    });
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &Widget::updatePosition);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &Widget::updateDuration);
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &Widget::updateState);
    connect(progressSlider, &QSlider::sliderMoved, this, &Widget::setPosition);
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
                QListWidgetItem *item = new QListWidgetItem(QString("%1 - %2").arg(songName, artistName));
                item->setData(Qt::UserRole, songObj["id"].toVariant());
                resultList->addItem(item);
            }
        }
    }
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
                QString imageUrl = songObj["album"].toObject()["picUrl"].toString() + "?param=300y300";
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

void Widget::onApiError(const QString &errorString)
{
    searchButton->setEnabled(true);
    searchButton->setText("搜索");
    QMessageBox::critical(this, "网络错误", errorString);
}

void Widget::onResultItemDoubleClicked(QListWidgetItem *item)
{
    qint64 songId = item->data(Qt::UserRole).toLongLong();
    
    // 获取播放链接并播放
    QString urlString = QString("https://music.163.com/song/media/outer/url?id=%1.mp3").arg(songId);
    mediaPlayer->setSource(QUrl(urlString));
    mediaPlayer->play();

    // 获取歌词和详情
    apiManager->getLyric(songId);
    apiManager->getSongDetail(songId);

    // 切换到播放详情页
    mainStackedWidget->setCurrentWidget(playerPage);
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
