#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QJsonDocument>
#include <QMediaPlayer>
#include <QMap>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include "playlistmanager.h" // 引入播放列表管理器

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
class PlaylistManager;
class QMenu;
class QWidgetAction;
class QAction;

class Widget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor widgetBackgroundColor READ getWidgetBackgroundColor WRITE setWidgetStyle)

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
    void onSongUrlReady(const QUrl &url);
    void onApiError(const QString &errorString);

    // 播放器相关
    void onResultItemDoubleClicked(QListWidgetItem *item);
    void onPlayPauseButtonClicked();
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void updateState(QMediaPlayer::PlaybackState state);
    void setPosition(int position);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

    // 新增：播放控制
    void playNextSong();
    void playPreviousSong();
    void changePlayMode();

    // 音量控制
    void onVolumeButtonClicked();

    // UI控制
    void onBackButtonClicked();

    // 分页控制
    void onPrevPageButtonClicked();
    void onNextPageButtonClicked();

    // 视图切换
    void onMainStackCurrentChanged(int index);

    // 托盘图标
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

    QColor getWidgetBackgroundColor() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void playSong(qint64 id); // 新增一个统一的播放歌曲函数
    void parseLyrics(const QString &lyricText);

    // 动态背景
    QColor extractDominantColor(const QPixmap &pixmap);
    void updateBackgroundColor(const QColor &color);
    bool isColorDark(const QColor &color) const;
    void setWidgetStyle(const QColor &color);

    // UI 元素
    QLineEdit *searchInput;
    QPushButton *searchButton;
    QListWidget *resultList;
    QPushButton *prevPageButton;
    QPushButton *nextPageButton;
    QLabel *pageLabel;
    QWidget *paginationWidget; // 分页控件容器
    QPushButton *playPauseButton;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QPushButton *playModeButton; // 新增播放模式按钮
    QSlider *progressSlider;
    QLabel *timeLabel;
    QSlider *volumeSlider;
    QPushButton *volumeButton; // 新增音量按钮
    QMenu *volumeMenu;         // 新增音量菜单
    QWidgetAction *volumeAction; // 用于将Slider放入Menu
    QPushButton *backButton; // 新增返回按钮

    // 播放详情页
    QStackedWidget *mainStackedWidget;
    QWidget *playerPage;
    QLabel *songNameLabel; // 新增：歌曲名称标签
    QLabel *albumArtLabel;
    QLabel *lyricLabel;

    // 布局
    QVBoxLayout *mainLayout;
    QHBoxLayout *topLayout;
    QHBoxLayout *paginationLayout;

    // 媒体播放器
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    qint64 currentDuration;

    // API管理器
    ApiManager *apiManager;

    // 播放列表管理器
    PlaylistManager *playlistManager;
    QVector<Song> searchResultSongs; // 存储搜索结果歌曲列表
    
    // 歌词数据
    QMap<qint64, QString> lyricData;

    // 搜索与分页
    QString currentSearchKeywords;
    int currentPage;
    qint64 currentPlayingSongId;

    // 动态背景
    QPropertyAnimation *backgroundAnimation;
    QColor currentBackgroundColor;
    QPixmap originalAlbumArt;

    // 系统托盘
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QAction *showAction;
    QAction *quitAction;
};
#endif // WIDGET_H
