#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QJsonDocument>
#include <QMediaPlayer>
#include <QMap>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QMovie>
#include "core/playlistmanager.h" // 引入播放列表管理器

// 搜索源枚举声明
enum class SearchSource;

// 自定义加载动画控件
class LoadingSpinner : public QWidget
{
    Q_OBJECT

public:
    explicit LoadingSpinner(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    void start();
    void stop();

private:
    QLabel *m_label;
    QMovie *m_movie;
};

// 动态流动背景控件（苹果音乐风格）
class FlowingBackground : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal timeOffset READ timeOffset WRITE setTimeOffset)

public:
    explicit FlowingBackground(QWidget *parent = nullptr);
    void setColors(const QVector<QColor> &colors);
    qreal timeOffset() const { return m_timeOffset; }
    void setTimeOffset(qreal offset);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QColor> m_colors;
    qreal m_timeOffset = 0;
    
    struct Blob {
        QColor color;
        qreal x, y;        // 中心位置 (0-1)
        qreal radius;      // 半径
        qreal speedX;      // 移动速度
        qreal speedY;
        qreal phase;       // 相位偏移
    };
    QVector<Blob> m_blobs;
};

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
class QMediaDevices;
class ApiManager;
class PlaylistManager;
class QMenu;
class QWidgetAction;
class QAction;
class QComboBox;

// 悬浮灵动岛窗口
class FloatingIsland : public QWidget
{
    Q_OBJECT

public:
    explicit FloatingIsland(QWidget *parent = nullptr);
    void setSongInfo(const QString &name, const QString &artist, const QPixmap &cover);
    void setPlaying(bool playing);
    void setPosition(qint64 position, qint64 duration);
    void updateBackground(); // 更新模糊背景

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void moveEvent(QMoveEvent *event) override;

private slots:
    void onPrevClicked();
    void onPlayPauseClicked();
    void onNextClicked();

signals:
    void prevClicked();
    void playPauseClicked();
    void nextClicked();
    void expandClicked();

private:
    QLabel *coverLabel;
    QLabel *songNameLabel;
    QLabel *artistLabel;
    QPushButton *prevBtn;
    QPushButton *playPauseBtn;
    QPushButton *nextBtn;
    bool isPlaying;
    bool isHovering;
    bool isDragging;
    QPoint dragStartPos;
    QPoint windowStartPos;
    QPoint originalPos; // 原始位置，用于双击复原
    QPixmap blurredBackground; // 模糊背景
};

class Widget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor widgetBackgroundColor READ getWidgetBackgroundColor WRITE setWidgetStyle)

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    // 网络相关 - 网易云音乐
    void onSearchButtonClicked();
    void onSearchFinished(const QJsonDocument &json);
    void onLyricFinished(const QJsonDocument &json);
    void onSongDetailFinished(const QJsonDocument &json);
    void onImageDownloaded(const QByteArray &data);
    void onSongUrlReady(const QUrl &url);

    // 网络相关 - Bilibili
    void onBilibiliSearchFinished(const QJsonDocument &json);
    void onBilibiliVideoInfoFinished(const QJsonDocument &json);
    void onBilibiliAudioUrlReady(const QUrl &url);
    void onBilibiliAudioDataReady(const QByteArray &data);
    void onBilibiliAudioFileReady(const QString &filePath);
    void onBilibiliImageDownloaded(const QByteArray &data);

    void onApiError(const QString &errorString);
    void onMediaPlayerError(QMediaPlayer::Error error, const QString &errorString);

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

    void updateVolumeIcon(int volume);

    // 搜索源切换
    void onSearchSourceChanged(int index);

    // 音量控制
    void onVolumeButtonClicked();

    // 悬浮窗控制
    void onMinimizeButtonClicked();
    void onFloatingExpandClicked();

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
    void playSong(qint64 id); // 播放网易云音乐歌曲
    void playBilibiliVideo(const QString &bvid); // 播放Bilibili视频
    void parseLyrics(const QString &lyricText);

    // 动态背景
    QColor extractDominantColor(const QPixmap &pixmap);
    QVector<QColor> extractPaletteColors(const QPixmap &pixmap, int colorCount = 3);
    void updateBackgroundColor(const QColor &color);
    void updateBackgroundWithPalette(const QVector<QColor> &colors);
    bool isColorDark(const QColor &color) const;
    void setWidgetStyle(const QColor &color);
    void setWidgetStyleWithPalette(const QVector<QColor> &colors);

    // UI 元素
    QLineEdit *searchInput;
    QPushButton *searchButton;
    QListWidget *resultList;
    QComboBox *searchSourceCombo; // 搜索源选择
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
    QPushButton *minimizeButton; // 新增缩小按钮
    FloatingIsland *floatingIsland; // 新增悬浮灵动岛

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

    // 加载动画
    LoadingSpinner *loadingSpinner;

    // 媒体播放器
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    QMediaDevices *mediaDevices;
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
    QString currentBvid; // 当前播放的Bilibili视频BV号
    QUrl currentBilibiliAudioUrl; // 当前Bilibili音频URL
    SearchSource currentSearchSource; // 当前搜索源

    // 动态背景
    QPropertyAnimation *backgroundAnimation;
    QColor currentBackgroundColor;
    QPixmap originalAlbumArt;
    FlowingBackground *flowingBackground; // 流动背景控件
    QPropertyAnimation *flowAnimation; // 流动动画
    QVector<QColor> currentPalette; // 当前调色板

    // 系统托盘
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QAction *showAction;
    QAction *quitAction;
};
#endif // WIDGET_H
