#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QJsonDocument>
#include <QMediaPlayer>
#include <QMap>

// 前置声明
class QLineEdit;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QSlider;
class QHBoxLayout;
class QVBoxLayout;
class QStackedWidget;
class QAudioOutput;
class ApiManager;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    // 网络相关
    void onSearchButtonClicked();
    void onSearchFinished(const QJsonDocument &json);
    void onLyricFinished(const QJsonDocument &json);
    void onSongDetailFinished(const QJsonDocument &json);
    void onImageDownloaded(const QByteArray &data);
    void onApiError(const QString &errorString);

    // 播放器相关
    void onResultItemDoubleClicked(QListWidgetItem *item);
    void onPlayPauseButtonClicked();
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void updateState(QMediaPlayer::PlaybackState state);
    void setPosition(int position);

private:
    void parseLyrics(const QString &lyricText);

    // UI 元素
    QLineEdit *searchInput;
    QPushButton *searchButton;
    QListWidget *resultList;
    QPushButton *playPauseButton;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QSlider *progressSlider;
    QLabel *timeLabel;
    QSlider *volumeSlider;
    
    // 播放详情页
    QStackedWidget *mainStackedWidget;
    QWidget *playerPage;
    QLabel *albumArtLabel;
    QLabel *lyricLabel;

    // 布局
    QVBoxLayout *mainLayout;
    QHBoxLayout *topLayout;
    QHBoxLayout *bottomLayout;

    // 媒体播放器
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    qint64 currentDuration;

    // API管理器
    ApiManager *apiManager;
    
    // 歌词数据
    QMap<qint64, QString> lyricData;
};
#endif // WIDGET_H
