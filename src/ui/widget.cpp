#include "widget.h"
#include "core/apimanager.h"
#include "core/playlistmanager.h" // 集成播放列表
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QBuffer>
#include <QStackedWidget>
#include <QAudioOutput>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QRegularExpression>
#include <QPixmap>
#include <QFile>
#include <QFont>
#include <QMenu>
#include <QWidgetAction>
#include <QDebug>
#include <QComboBox>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QEasingCurve>
#include <QtMath>
#include <QMouseEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QGraphicsBlurEffect>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

// --- FloatingIsland 实现 ---
FloatingIsland::FloatingIsland(QWidget *parent)
    : QWidget(parent), isPlaying(false), isHovering(false), isDragging(false)
{
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(300, 45);

    // 创建控件
    coverLabel = new QLabel(this);
    coverLabel->setFixedSize(32, 32);
    coverLabel->setScaledContents(true);

    songNameLabel = new QLabel("未在播放", this);
    songNameLabel->setStyleSheet("color: white; font-weight: bold; font-size: 11px; background: transparent;");
    songNameLabel->setMaximumWidth(130);

    artistLabel = new QLabel("", this);
    artistLabel->setStyleSheet("color: rgba(255,255,255,180); font-size: 10px; background: transparent;");
    artistLabel->setMaximumWidth(130);

    prevBtn = new QPushButton(this);
    prevBtn->setIcon(QIcon(":/icons/previous.png"));
    prevBtn->setIconSize(QSize(14, 14));
    prevBtn->setFixedSize(22, 22);
    prevBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.15); border: none; border-radius: 11px; }"
                          "QPushButton:hover { background: rgba(255,255,255,0.25); }");

    playPauseBtn = new QPushButton(this);
    playPauseBtn->setIcon(QIcon(":/icons/play.png"));
    playPauseBtn->setIconSize(QSize(16, 16));
    playPauseBtn->setFixedSize(22, 22);
    playPauseBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.2); border: none; border-radius: 11px; }"
                               "QPushButton:hover { background: rgba(255,255,255,0.3); }");

    nextBtn = new QPushButton(this);
    nextBtn->setIcon(QIcon(":/icons/next.png"));
    nextBtn->setIconSize(QSize(14, 14));
    nextBtn->setFixedSize(22, 22);
    nextBtn->setStyleSheet("QPushButton { background: rgba(255,255,255,0.15); border: none; border-radius: 11px; }"
                          "QPushButton:hover { background: rgba(255,255,255,0.25); }");

    // 布局 - 增大边距
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 6, 10, 6);
    mainLayout->setSpacing(8);
    mainLayout->addWidget(coverLabel);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);
    infoLayout->addWidget(songNameLabel);
    infoLayout->addWidget(artistLabel);
    mainLayout->addLayout(infoLayout);
    mainLayout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);
    btnLayout->addWidget(prevBtn);
    btnLayout->addWidget(playPauseBtn);
    btnLayout->addWidget(nextBtn);
    mainLayout->addLayout(btnLayout);

    setLayout(mainLayout);

    // 连接信号
    connect(prevBtn, &QPushButton::clicked, this, &FloatingIsland::onPrevClicked);
    connect(playPauseBtn, &QPushButton::clicked, this, &FloatingIsland::onPlayPauseClicked);
    connect(nextBtn, &QPushButton::clicked, this, &FloatingIsland::onNextClicked);
}

void FloatingIsland::setSongInfo(const QString &name, const QString &artist, const QPixmap &cover)
{
    songNameLabel->setText(name);
    artistLabel->setText(artist);
    if (!cover.isNull()) {
        // 创建圆角封面
        QPixmap scaledCover = cover.scaled(32, 32, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

        // 创建圆角蒙版
        QPixmap roundedCover(32, 32);
        roundedCover.fill(Qt::transparent);

        QPainter painter(&roundedCover);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addRoundedRect(0, 0, 32, 32, 6, 6);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, scaledCover);
        painter.end();

        coverLabel->setPixmap(roundedCover);
    }
    update();
}

void FloatingIsland::setPlaying(bool playing)
{
    isPlaying = playing;
    playPauseBtn->setIcon(QIcon(playing ? ":/icons/pause.png" : ":/icons/play.png"));
}

void FloatingIsland::setPosition(qint64 position, qint64 duration)
{
    Q_UNUSED(position);
    Q_UNUSED(duration);
    // 可扩展：显示进度
}

void FloatingIsland::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制圆角背景
    QPainterPath path;
    path.addRoundedRect(rect(), 21, 21);

    // 绘制模糊背景
    if (!blurredBackground.isNull()) {
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, blurredBackground);
        // 添加半透明遮罩增强效果 - 更黑的颜色
        QColor overlayColor = isHovering ? QColor(15, 15, 18, 200) : QColor(5, 5, 8, 220);
        painter.fillPath(path, overlayColor);
    } else {
        QColor bgColor = isHovering ? QColor(25, 25, 28, 250) : QColor(10, 10, 15, 245);
        painter.fillPath(path, bgColor);
    }
}

void FloatingIsland::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    updateBackground();
}

void FloatingIsland::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event);
    updateBackground();
}

void FloatingIsland::updateBackground()
{
    // 截取窗口背后的屏幕区域
    QScreen *screen = QGuiApplication::screenAt(pos());
    if (!screen) screen = QGuiApplication::primaryScreen();

    QRect windowRect(pos(), size());
    QPixmap screenshot = screen->grabWindow(0, windowRect.x(), windowRect.y(), windowRect.width(), windowRect.height());

    // 应用高斯模糊 - 更大的模糊半径
    if (!screenshot.isNull()) {
        QGraphicsScene scene;
        QGraphicsPixmapItem item;
        item.setPixmap(screenshot);

        QGraphicsBlurEffect blur;
        blur.setBlurRadius(50);
        blur.setBlurHints(QGraphicsBlurEffect::QualityHint);
        item.setGraphicsEffect(&blur);

        scene.addItem(&item);

        blurredBackground = QPixmap(size());
        blurredBackground.fill(Qt::transparent);

        QPainter painter(&blurredBackground);
        scene.render(&painter, QRectF(), QRectF(0, 0, screenshot.width(), screenshot.height()));
        painter.end();

        update();
    }
}

void FloatingIsland::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        dragStartPos = event->globalPosition().toPoint();
        windowStartPos = pos();
    }
}

void FloatingIsland::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging) {
        QPoint currentPos = event->globalPosition().toPoint();
        QPoint delta = currentPos - dragStartPos;
        move(windowStartPos + delta);
    }
}

void FloatingIsland::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
    }
}

void FloatingIsland::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    // 双击展开恢复主窗口
    emit expandClicked();
}

void FloatingIsland::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    isHovering = true;
    update();
}

void FloatingIsland::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    isHovering = false;
    update();
}

void FloatingIsland::onPrevClicked()
{
    emit prevClicked();
}

void FloatingIsland::onPlayPauseClicked()
{
    emit playPauseClicked();
}

void FloatingIsland::onNextClicked()
{
    emit nextClicked();
}

// --- LoadingSpinner 实现 ---
LoadingSpinner::LoadingSpinner(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(28, 28);
    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setGeometry(0, 0, 28, 28);

    m_movie = new QMovie(":/icons/loading.gif");
    m_movie->setScaledSize(QSize(28, 28));
    m_label->setMovie(m_movie);
    hide();
}

QSize LoadingSpinner::sizeHint() const
{
    return QSize(28, 28);
}

void LoadingSpinner::start()
{
    m_movie->start();
    show();
}

void LoadingSpinner::stop()
{
    m_movie->stop();
    hide();
}

// --- FlowingBackground 实现 ---

FlowingBackground::FlowingBackground(QWidget *parent)
    : QWidget(parent)
{
    // 初始化默认颜色
    m_colors = { QColor(80, 60, 140), QColor(60, 80, 120), QColor(40, 60, 100) };
}

void FlowingBackground::setColors(const QVector<QColor> &colors)
{
    m_colors = colors;
    m_blobs.clear();
    
    if (colors.isEmpty()) return;
    
    // 为每种颜色创建一个"blob"
    for (int i = 0; i < colors.size(); ++i) {
        Blob blob;
        blob.color = colors[i];
        blob.x = 0.2 + (i % 3) * 0.3;  // 分散初始位置
        blob.y = 0.2 + (i / 3) * 0.3;
        blob.radius = 0.4 + (i % 2) * 0.2;  // 不同大小
        blob.speedX = 0.0003 + i * 0.0001;  // 不同速度
        blob.speedY = 0.0002 + i * 0.00015;
        blob.phase = i * 1.5;  // 相位偏移
        m_blobs.append(blob);
    }
    
    update();
}

void FlowingBackground::setTimeOffset(qreal offset)
{
    m_timeOffset = offset;
    update();
}

void FlowingBackground::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    QRectF rect = this->rect();
    qreal w = rect.width();
    qreal h = rect.height();
    
    // 深色底色
    painter.fillRect(rect, QColor(20, 20, 25));
    
    // 绘制流动的颜色块
    for (const Blob &blob : m_blobs) {
        // 使用正弦函数创建平滑的移动轨迹
        qreal t = m_timeOffset + blob.phase;
        qreal x = blob.x + qSin(t * blob.speedX * 1000) * 0.3;
        qreal y = blob.y + qCos(t * blob.speedY * 1000) * 0.3;
        
        // 确保在边界内
        x = qBound(0.1, x, 0.9);
        y = qBound(0.1, y, 0.9);
        
        // 转换为像素坐标
        qreal cx = x * w;
        qreal cy = y * h;
        qreal radius = blob.radius * qMax(w, h);
        
        // 创建径向渐变
        QRadialGradient gradient(cx, cy, radius);
        QColor color = blob.color;
        gradient.setColorAt(0, QColor(color.red(), color.green(), color.blue(), 180));
        gradient.setColorAt(0.5, QColor(color.red(), color.green(), color.blue(), 80));
        gradient.setColorAt(1, QColor(color.red(), color.green(), color.blue(), 0));
        
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(cx, cy), radius, radius);
    }
    
    // 添加暗色遮罩提升文字可读性
    QLinearGradient overlay(0, 0, 0, h);
    overlay.setColorAt(0, QColor(0, 0, 0, 80));
    overlay.setColorAt(0.5, QColor(0, 0, 0, 40));
    overlay.setColorAt(1, QColor(0, 0, 0, 100));
    painter.fillRect(rect, overlay);
}

// --- Widget 实现 ---

Widget::Widget(QWidget *parent)
    : QWidget(parent), currentDuration(0)
{
    this->setObjectName("mainWidget");
    // --- 新增：播放列表管理器初始化 ---
    playlistManager = new PlaylistManager(this);

    // --- 业务逻辑变量初始化 ---
    currentPage = 1;
    currentPlayingSongId = -1;
    currentSearchSource = SearchSource::NetEase; // 默认网易云音乐

    // --- 动态背景初始化 ---
    currentBackgroundColor = QColor(51, 51, 51);
    backgroundAnimation = new QPropertyAnimation(this, "widgetBackgroundColor", this);
    backgroundAnimation->setDuration(800);
    backgroundAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    // 流动背景控件
    flowingBackground = new FlowingBackground(this);
    flowingBackground->lower(); // 放到最底层
    
    // 流动动画
    flowAnimation = new QPropertyAnimation(flowingBackground, "timeOffset", this);
    flowAnimation->setDuration(20000); // 20秒一个周期
    flowAnimation->setStartValue(0.0);
    flowAnimation->setEndValue(100.0);
    flowAnimation->setLoopCount(-1); // 无限循环
    flowAnimation->setEasingCurve(QEasingCurve::Linear);

    // --- 加载动画初始化 ---
    loadingSpinner = new LoadingSpinner(this);
    loadingSpinner->hide(); // 默认隐藏

    // --- UI 控件初始化 ---
    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("输入歌名或歌手...");
    searchButton = new QPushButton;
    searchButton->setIcon(QIcon(":/icons/search.png"));
    searchButton->setIconSize(QSize(20, 20));
    searchButton->setToolTip("搜索");
    searchButton->setFixedSize(28, 28);
    backButton = new QPushButton;
    backButton->setIcon(QIcon(":/icons/back.png"));
    backButton->setIconSize(QSize(20, 20));
    backButton->setToolTip("返回");
    backButton->setFixedSize(28, 28);
    backButton->setVisible(false); // 默认隐藏
    resultList = new QListWidget;

    // 搜索源选择下拉框
    searchSourceCombo = new QComboBox;
    searchSourceCombo->addItem("网易云音乐");
    searchSourceCombo->addItem("Bilibili");
    searchSourceCombo->setFixedWidth(90);
    playPauseButton = new QPushButton;
    playPauseButton->setIcon(QIcon(":/icons/play.png"));
    playPauseButton->setIconSize(QSize(24, 24));
    playPauseButton->setFixedSize(28, 28);
    prevButton = new QPushButton;
    prevButton->setIcon(QIcon(":/icons/previous.png"));
    prevButton->setIconSize(QSize(20, 20));
    prevButton->setFixedSize(28, 28);
    nextButton = new QPushButton;
    nextButton->setIcon(QIcon(":/icons/next.png"));
    nextButton->setIconSize(QSize(20, 20));
    nextButton->setFixedSize(28, 28);
    playModeButton = new QPushButton;
    playModeButton->setIcon(QIcon(":/icons/loop-list.png"));
    playModeButton->setIconSize(QSize(20, 20));
    playModeButton->setFixedSize(28, 28);
    progressSlider = new QSlider(Qt::Horizontal);
    timeLabel = new QLabel("00:00 / 00:00");

    // --- 分页控件 ---
    paginationWidget = new QWidget;
    prevPageButton = new QPushButton;
    prevPageButton->setIcon(QIcon(":/icons/previous-page.png"));
    prevPageButton->setIconSize(QSize(20, 20));
    prevPageButton->setToolTip("上一页");
    prevPageButton->setFixedSize(28, 28);
    nextPageButton = new QPushButton;
    nextPageButton->setIcon(QIcon(":/icons/next-page.png"));
    nextPageButton->setIconSize(QSize(20, 20));
    nextPageButton->setToolTip("下一页");
    nextPageButton->setFixedSize(28, 28);
    pageLabel = new QLabel("第 1 页");
    prevPageButton->setEnabled(false); // 初始时禁用
    nextPageButton->setEnabled(false); // 初始时禁用

    // --- 音量控制 ---
    volumeButton = new QPushButton;
    volumeButton->setIcon(QIcon(":/icons/volume-high.png"));
    volumeButton->setIconSize(QSize(20, 20));
    volumeButton->setFixedSize(28, 28);

    // --- 缩小按钮 ---
    minimizeButton = new QPushButton;
    minimizeButton->setIcon(QIcon(":/icons/minimize.png"));
    minimizeButton->setIconSize(QSize(20, 20));
    minimizeButton->setToolTip("迷你模式");
    minimizeButton->setFixedSize(28, 28);

    volumeSlider = new QSlider(Qt::Vertical); // 设置为垂直
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);
    volumeSlider->setFixedHeight(100); // 设置高度
    updateVolumeIcon(50);

    volumeMenu = new QMenu(this);
    volumeAction = new QWidgetAction(this);
    volumeAction->setDefaultWidget(volumeSlider);
    volumeMenu->addAction(volumeAction);

    // 播放详情页
    playerPage = new QWidget;
    songNameLabel = new QLabel("歌曲名称"); // 初始化
    songNameLabel->setAlignment(Qt::AlignCenter);
    songNameLabel->setWordWrap(true);  // 启用自动换行
    songNameLabel->setMaximumWidth(400);  // 限制最大宽度
    QFont songNameFont = songNameLabel->font();
    songNameFont.setPointSize(16);
    songNameFont.setBold(true);
    songNameLabel->setFont(songNameFont);

    albumArtLabel = new QLabel;
    albumArtLabel->setScaledContents(true);
    albumArtLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    albumArtLabel->setAlignment(Qt::AlignCenter);

    lyricLabel = new QLabel("欢迎使用 Melody");
    lyricLabel->setObjectName("lyricLabel");
    lyricLabel->setAlignment(Qt::AlignCenter);
    lyricLabel->setWordWrap(true);
    QFont lyricFont = lyricLabel->font();
    lyricFont.setPointSize(14);
    lyricLabel->setFont(lyricFont);

    QVBoxLayout *playerPageLayout = new QVBoxLayout(playerPage);
    playerPageLayout->addWidget(songNameLabel, 0, Qt::AlignCenter); // 添加到布局
    playerPageLayout->addWidget(albumArtLabel, 1, Qt::AlignCenter); // 添加stretch因子，让封面占据主要空间
    playerPageLayout->addWidget(lyricLabel, 0, Qt::AlignCenter);
    playerPageLayout->setStretch(0, 0); // 歌曲名不拉伸
    playerPageLayout->setStretch(1, 1); // 封面可拉伸
    playerPageLayout->setStretch(2, 0); // 歌词不拉伸

    // 主堆叠窗口
    mainStackedWidget = new QStackedWidget;
    mainStackedWidget->addWidget(resultList);
    mainStackedWidget->addWidget(playerPage);
    mainStackedWidget->setCurrentWidget(resultList); // 默认显示搜索页
    // 设置大小策略为Expanding，确保两个页面占用相同的空间
    mainStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // --- 布局设置 ---
    topLayout = new QHBoxLayout;
    topLayout->addWidget(backButton);
    topLayout->addWidget(searchSourceCombo); // 添加搜索源选择
    topLayout->addWidget(searchInput);
    topLayout->addWidget(searchButton);

    paginationLayout = new QHBoxLayout(paginationWidget);
    paginationLayout->addStretch();
    paginationLayout->addWidget(prevPageButton);
    paginationLayout->addWidget(pageLabel);
    paginationLayout->addWidget(nextPageButton);
    paginationLayout->addStretch();
    paginationWidget->setLayout(paginationLayout);

    // --- 底部控制区布局 ---
    // 第一行：进度条和时间
    QHBoxLayout *progressLayout = new QHBoxLayout;
    progressLayout->addWidget(progressSlider);
    progressLayout->addWidget(timeLabel);

    // 第二行：控制按钮
    QHBoxLayout *controlsLayout = new QHBoxLayout;
    // 左侧占位符，平衡右边的 minimizeButton
    QWidget *leftSpacer = new QWidget;
    leftSpacer->setFixedSize(28, 28);
    controlsLayout->addWidget(leftSpacer);
    controlsLayout->addWidget(playModeButton);
    controlsLayout->addStretch();
    controlsLayout->addWidget(prevButton);
    controlsLayout->addWidget(playPauseButton);
    controlsLayout->addWidget(loadingSpinner);  // 加载动画控件
    controlsLayout->addWidget(nextButton);
    controlsLayout->addStretch();
    controlsLayout->addWidget(minimizeButton); // 缩小按钮
    controlsLayout->addWidget(volumeButton);

    // 垂直整合底部所有控件
    QVBoxLayout *bottomContainerLayout = new QVBoxLayout;
    bottomContainerLayout->addLayout(progressLayout);
    bottomContainerLayout->addLayout(controlsLayout);

    mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(mainStackedWidget);
    mainLayout->addWidget(paginationWidget); // 添加分页控件容器
    mainLayout->addLayout(bottomContainerLayout);

    setLayout(mainLayout);
    setWindowTitle("Melody");
    setWindowIcon(QIcon(":/logo.png"));
    resize(350, 450);

    // --- 样式表设置 ---
    setWidgetStyle(currentBackgroundColor);

    // --- 后端对象初始化 ---
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaDevices = new QMediaDevices(this);
    mediaPlayer->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.5);
    
    // 设置默认音频输出设备
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
    if (!defaultDevice.isNull()) {
        audioOutput->setDevice(defaultDevice);
    }
    
    apiManager = new ApiManager(this);

    // 初始化播放看门狗定时器（用于检测播放卡住）
    playbackWatchdog = new QTimer(this);
    playbackWatchdog->setInterval(5000); // 每5秒检查一次
    connect(playbackWatchdog, &QTimer::timeout, this, &Widget::checkPlaybackHealth);

    // --- 信号与槽连接 ---
    connect(mainStackedWidget, &QStackedWidget::currentChanged, this, &Widget::onMainStackCurrentChanged);
    connect(backButton, &QPushButton::clicked, this, &Widget::onBackButtonClicked);
    connect(prevPageButton, &QPushButton::clicked, this, &Widget::onPrevPageButtonClicked);
    connect(nextPageButton, &QPushButton::clicked, this, &Widget::onNextPageButtonClicked);
    connect(searchButton, &QPushButton::clicked, this, &Widget::onSearchButtonClicked);
    connect(searchInput, &QLineEdit::returnPressed, this, &Widget::onSearchButtonClicked);

    // 搜索源切换
    connect(searchSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Widget::onSearchSourceChanged);

    // 网易云音乐API信号
    connect(apiManager, &ApiManager::searchFinished, this, &Widget::onSearchFinished);
    connect(apiManager, &ApiManager::lyricFinished, this, &Widget::onLyricFinished);
    connect(apiManager, &ApiManager::songDetailFinished, this, &Widget::onSongDetailFinished);
    connect(apiManager, &ApiManager::imageDownloaded, this, &Widget::onImageDownloaded);
    connect(apiManager, &ApiManager::songUrlReady, this, &Widget::onSongUrlReady);

    // Bilibili API信号
    connect(apiManager, &ApiManager::bilibiliSearchFinished, this, &Widget::onBilibiliSearchFinished);
    connect(apiManager, &ApiManager::bilibiliVideoInfoFinished, this, &Widget::onBilibiliVideoInfoFinished);
    connect(apiManager, &ApiManager::bilibiliAudioUrlReady, this, &Widget::onBilibiliAudioUrlReady);
    connect(apiManager, &ApiManager::bilibiliAudioDataReady, this, &Widget::onBilibiliAudioDataReady);
    connect(apiManager, &ApiManager::bilibiliAudioFileReady, this, &Widget::onBilibiliAudioFileReady);
    connect(apiManager, &ApiManager::bilibiliImageDownloaded, this, &Widget::onBilibiliImageDownloaded);

    connect(apiManager, &ApiManager::error, this, &Widget::onApiError);
    connect(resultList, &QListWidget::itemDoubleClicked, this, &Widget::onResultItemDoubleClicked);
    connect(playPauseButton, &QPushButton::clicked, this, &Widget::onPlayPauseButtonClicked);
    connect(volumeButton, &QPushButton::clicked, this, &Widget::onVolumeButtonClicked); // 连接音量按钮
    connect(volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        audioOutput->setVolume(value / 100.0);
        updateVolumeIcon(value);
    });
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &Widget::updatePosition);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &Widget::updateDuration);
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &Widget::updateState);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &Widget::onMediaStatusChanged); // 监听播放结束
    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this, &Widget::onMediaPlayerError); // 监听播放错误
    connect(progressSlider, &QSlider::sliderMoved, this, &Widget::setPosition);

    // 监听音频输出设备变化
    connect(mediaDevices, &QMediaDevices::audioOutputsChanged, this, [this]() {
        QAudioDevice newDefault = QMediaDevices::defaultAudioOutput();
        if (!newDefault.isNull() && audioOutput->device() != newDefault) {
            audioOutput->setDevice(newDefault);
            qDebug() << "音频设备已切换到:" << newDefault.description();
        }
    });

    // 新增：连接上一曲/下一曲/播放模式按钮
    connect(prevButton, &QPushButton::clicked, this, &Widget::playPreviousSong);
    connect(nextButton, &QPushButton::clicked, this, &Widget::playNextSong);
    connect(playModeButton, &QPushButton::clicked, this, &Widget::changePlayMode);

    // --- 系统托盘初始化 ---
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/logo.png"));
    trayIcon->setToolTip("Melody");

    showAction = new QAction("显示窗口", this);
    connect(showAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction("退出", this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(showAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayIconMenu);
    // 不在这里 show()，而是在 closeEvent 中
    // trayIcon->show(); 

    connect(trayIcon, &QSystemTrayIcon::activated, this, &Widget::onTrayIconActivated);

    // --- 悬浮灵动岛初始化 ---
    floatingIsland = new FloatingIsland();
    connect(floatingIsland, &FloatingIsland::prevClicked, this, &Widget::playPreviousSong);
    connect(floatingIsland, &FloatingIsland::playPauseClicked, this, &Widget::onPlayPauseButtonClicked);
    connect(floatingIsland, &FloatingIsland::nextClicked, this, &Widget::playNextSong);
    connect(floatingIsland, &FloatingIsland::expandClicked, this, &Widget::onFloatingExpandClicked);
    connect(minimizeButton, &QPushButton::clicked, this, &Widget::onMinimizeButtonClicked);
}

Widget::~Widget()
{
    if (floatingIsland) {
        delete floatingIsland;
    }
}

void Widget::onSearchButtonClicked()
{
    currentSearchKeywords = searchInput->text();
    if (!currentSearchKeywords.isEmpty()) {
        currentPage = 1; // 每次新搜索都重置为第一页
        mainStackedWidget->setCurrentWidget(resultList);
        searchButton->setEnabled(false);
        searchButton->setToolTip("搜索中...");

        // 根据搜索源调用不同的API
        if (currentSearchSource == SearchSource::NetEase) {
            apiManager->searchSongs(currentSearchKeywords, 15, (currentPage - 1) * 15);
        } else {
            apiManager->searchBilibiliVideos(currentSearchKeywords, currentPage);
        }
    }
}

void Widget::onSearchSourceChanged(int index)
{
    currentSearchSource = (index == 0) ? SearchSource::NetEase : SearchSource::Bilibili;
    // 清空当前搜索结果
    resultList->clear();
    searchResultSongs.clear();
    playlistManager->addSongs({}); // 清空播放列表

    // 更新placeholder提示
    if (currentSearchSource == SearchSource::NetEase) {
        searchInput->setPlaceholderText("输入歌名或歌手...");
    } else {
        searchInput->setPlaceholderText("输入Bilibili视频关键词...");
    }
}

void Widget::onSearchFinished(const QJsonDocument &json)
{
    searchButton->setEnabled(true);
    searchButton->setToolTip("搜索");
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

                Song song;
                song.id = songId;
                song.name = songName;
                song.artist = artistName;
                song.source = SearchSource::NetEase;
                searchResultSongs.append(song);
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

void Widget::onBilibiliSearchFinished(const QJsonDocument &json)
{
    searchButton->setEnabled(true);
    searchButton->setToolTip("搜索");
    resultList->clear();
    searchResultSongs.clear();

    QJsonObject rootObj = json.object();
    int totalResults = 0;

    qDebug() << "Bilibili search response code:" << rootObj.value("code").toInt();
    qDebug() << "Bilibili search response message:" << rootObj.value("message").toString();

    if (rootObj.value("code").toInt() != 0) {
        QString message = rootObj.value("message").toString();
        QMessageBox::warning(this, "搜索失败", message.isEmpty() ? "Bilibili搜索失败" : message);
        return;
    }

    QJsonObject data = rootObj.value("data").toObject();
    totalResults = data.value("numResults").toInt();
    qDebug() << "Bilibili search total results:" << totalResults;

    // Bilibili搜索API的响应结构: data.result.video 包含视频列表
    QJsonObject resultObj = data.value("result").toObject();
    QJsonArray videosArray = resultObj.value("video").toArray();
    qDebug() << "Bilibili search videos count:" << videosArray.size();

    if (videosArray.isEmpty() && currentPage == 1) {
        QMessageBox::information(this, "无结果", "未找到相关视频。");
    }

    for (const QJsonValue &value : videosArray) {
        QJsonObject videoObj = value.toObject();

        QString bvid = videoObj["bvid"].toString();
        QString title = videoObj["title"].toString();
        // 去除HTML标签
        title.remove(QRegularExpression("<[^>]*>"));
        QString author = videoObj["author"].toString();
        QString pic = videoObj["pic"].toString();
        if (!pic.startsWith("http")) {
            pic = "https:" + pic;
        }
        int duration = videoObj["duration"].toString().split(":").first().toInt() * 60 +
                       videoObj["duration"].toString().split(":").last().toInt();

        QListWidgetItem *item = new QListWidgetItem(QString("[B站] %1 - %2").arg(title, author));
        item->setData(Qt::UserRole, bvid); // 使用bvid作为ID
        resultList->addItem(item);

        Song song;
        song.bvid = bvid;
        song.name = title;
        song.artist = author;
        song.picUrl = pic;
        song.duration = duration;
        song.source = SearchSource::Bilibili;
        searchResultSongs.append(song);
    }

    playlistManager->addSongs(searchResultSongs);

    // 更新分页控件状态 (Bilibili每页20个结果)
    int totalPages = (totalResults > 0) ? (totalResults + 19) / 20 : 0;
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

void Widget::onMinimizeButtonClicked()
{
    // 更新悬浮窗信息
    Song currentSong = playlistManager->getCurrentSong();
    if (!currentSong.name.isEmpty()) {
        floatingIsland->setSongInfo(currentSong.name, currentSong.artist, originalAlbumArt);
    }
    floatingIsland->setPlaying(mediaPlayer->playbackState() == QMediaPlayer::PlayingState);

    // 定位到屏幕顶部中央
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int x = screenGeometry.x() + (screenGeometry.width() - floatingIsland->width()) / 2;
    int y = screenGeometry.y() + 10;
    floatingIsland->move(x, y);
    floatingIsland->show();

    // 隐藏主窗口
    this->hide();
    trayIcon->show();
}

void Widget::onFloatingExpandClicked()
{
    floatingIsland->hide();
    this->showNormal();
    this->activateWindow();
    trayIcon->hide();
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

        // 更新悬浮窗封面
        Song currentSong = playlistManager->getCurrentSong();
        floatingIsland->setSongInfo(currentSong.name, currentSong.artist, originalAlbumArt);

        // 使用调色板提取和模糊背景（苹果音乐风格）
        QVector<QColor> palette = extractPaletteColors(pixmap, 3);
        updateBackgroundWithPalette(palette);
    }
}

void Widget::onSongUrlReady(const QUrl &url)
{
    mediaPlayer->setSource(url);
    mediaPlayer->play();

    // 启动看门狗定时器
    playbackWatchdog->start();
}

void Widget::onBilibiliVideoInfoFinished(const QJsonDocument &json)
{
    QJsonObject rootObj = json.object();
    if (rootObj.value("code").toInt() != 0) {
        qDebug() << "获取Bilibili视频信息失败:" << rootObj.value("message").toString();
        return;
    }

    QJsonObject data = rootObj.value("data").toObject();
    QString bvid = data.value("bvid").toString();
    qint64 cid = data.value("cid").toVariant().toLongLong();
    QString pic = data.value("pic").toString();

    // 更新当前歌曲的cid
    Song currentSong = playlistManager->getCurrentSong();
    if (currentSong.bvid == bvid) {
        // 找到并更新cid
        for (int i = 0; i < searchResultSongs.size(); ++i) {
            if (searchResultSongs[i].bvid == bvid) {
                searchResultSongs[i].cid = cid;
                break;
            }
        }

        // 下载封面图
        if (!pic.isEmpty()) {
            if (!pic.startsWith("http")) {
                pic = "https:" + pic;
            }
            apiManager->downloadBilibiliImage(QUrl(pic));
        }

        // 获取音频URL
        apiManager->getBilibiliAudioUrl(bvid, cid);
    }
}

void Widget::onBilibiliAudioUrlReady(const QUrl &url)
{
    // 方案1：先尝试直接播放
    mediaPlayer->setSource(url);
    mediaPlayer->play();

    // 保存URL，如果播放失败会用到
    currentBilibiliAudioUrl = url;

    // 启动看门狗定时器
    playbackWatchdog->start();

    // 注意：加载动画在onMediaPlayerError或onBilibiliAudioFileReady中隐藏
    // 因为直接播放可能失败（403错误）
}

void Widget::onBilibiliAudioDataReady(const QByteArray &data)
{
    // 清理之前的音频缓冲区（如果有）
    if (currentAudioBuffer) {
        currentAudioBuffer->close();
        currentAudioBuffer->deleteLater();
        currentAudioBuffer = nullptr;
    }

    // 使用 QBuffer 播放下载的音频数据
    currentAudioBuffer = new QBuffer();
    currentAudioBuffer->setData(data);
    currentAudioBuffer->open(QIODevice::ReadOnly);

    // 设置媒体源为缓冲区
    mediaPlayer->setSourceDevice(currentAudioBuffer);
    mediaPlayer->play();

    // 启动看门狗定时器
    playbackWatchdog->start();
}

void Widget::onBilibiliAudioFileReady(const QString &filePath)
{
    // 隐藏加载动画，显示播放按钮
    loadingSpinner->stop();
    playPauseButton->show();

    // 清理之前的临时文件（如果有）
    if (!currentTempAudioFile.isEmpty() && QFile::exists(currentTempAudioFile)) {
        QFile::remove(currentTempAudioFile);
        qDebug() << "Removed previous temporary audio file:" << currentTempAudioFile;
    }

    // 保存当前临时文件路径
    currentTempAudioFile = filePath;

    // 使用临时文件播放
    mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
    mediaPlayer->play();

    // 启动看门狗定时器
    playbackWatchdog->start();
}

void Widget::onBilibiliImageDownloaded(const QByteArray &data)
{
    QPixmap pixmap;
    if (pixmap.loadFromData(data)) {
        originalAlbumArt = pixmap;
        int size = qMin(this->width(), this->height()) * 0.6;
        albumArtLabel->setPixmap(originalAlbumArt.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        // 更新悬浮窗封面
        Song currentSong = playlistManager->getCurrentSong();
        floatingIsland->setSongInfo(currentSong.name, currentSong.artist, originalAlbumArt);

        // 使用调色板提取和模糊背景（苹果音乐风格）
        QVector<QColor> palette = extractPaletteColors(pixmap, 3);
        updateBackgroundWithPalette(palette);
    }
}

void Widget::onApiError(const QString &errorString)
{
    // 检查错误是否与获取歌曲URL有关
    if (errorString.contains("mp3") && currentPlayingSongId != -1) {
        qDebug() << "API URL failed, trying fallback direct link for song ID:" << currentPlayingSongId;
        QString fallbackUrl = QString("https://music.163.com/song/media/outer/url?id=%1.mp3").arg(currentPlayingSongId);
        mediaPlayer->setSource(QUrl(fallbackUrl));
        mediaPlayer->play();
        return; // 尝试备用链接，不显示错误弹窗
    }

    searchButton->setEnabled(true);
    searchButton->setToolTip("搜索");
    QMessageBox::critical(this, "网络错误", errorString);
}

void Widget::onMediaPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    // 检查是否是访问被拒绝错误（403）
    if (error == QMediaPlayer::ResourceError && !currentBilibiliAudioUrl.isEmpty()) {
        qDebug() << "Direct playback failed (likely 403), switching to download mode for:" << currentBilibiliAudioUrl.toString();

        // 停止当前播放
        mediaPlayer->stop();

        // 使用备用方案：下载音频文件
        apiManager->downloadBilibiliAudio(currentBilibiliAudioUrl);

        // 清空当前URL，避免重复尝试
        currentBilibiliAudioUrl.clear();
    } else {
        // 其他错误，显示错误信息并隐藏加载动画
        loadingSpinner->stop();
        playPauseButton->show();
        qDebug() << "Media player error:" << error << errorString;
    }
}

void Widget::onBackButtonClicked()
{
    mainStackedWidget->setCurrentWidget(resultList);
}

void Widget::onResultItemDoubleClicked(QListWidgetItem *item)
{
    int index = resultList->row(item);
    Song clickedSong = searchResultSongs.at(index);

    // 检查点击的歌曲是否就是当前正在播放的歌曲
    bool isSameSong = (clickedSong.source == SearchSource::NetEase && clickedSong.id == currentPlayingSongId) ||
                      (clickedSong.source == SearchSource::Bilibili && clickedSong.bvid == currentBvid);

    if (isSameSong && mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        // 如果是，并且播放器不是停止状态，则只切换回播放界面
        songNameLabel->setText(clickedSong.name);
        mainStackedWidget->setCurrentWidget(playerPage);
    } else {
        // 否则，按正常流程播放新歌曲
        playlistManager->setCurrentIndex(index);

        Song currentSong = playlistManager->getCurrentSong();
        if (currentSong.source == SearchSource::Bilibili) {
            playBilibiliVideo(currentSong.bvid);
        } else if (currentSong.id != -1) {
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
        playPauseButton->setIcon(QIcon(":/icons/pause.png"));

        if (loadingSpinner->isVisible()) {
            loadingSpinner->stop();
            playPauseButton->show();
        }

        // 启动看门狗定时器
        playbackWatchdog->start();
    } else {
        playPauseButton->setIcon(QIcon(":/icons/play.png"));

        // 停止状态时停止看门狗
        if (state == QMediaPlayer::StoppedState) {
            playbackWatchdog->stop();
        }
    }

    // 更新悬浮窗状态
    floatingIsland->setPlaying(state == QMediaPlayer::PlayingState);
}

void Widget::setPosition(int position)
{
    mediaPlayer->setPosition(position);
}

// --- 新增的私有和槽函数实现 ---

void Widget::cleanupPreviousPlayback()
{
    // 停止播放器
    if (mediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        mediaPlayer->stop();
    }

    // 清理音频缓冲区
    if (currentAudioBuffer) {
        currentAudioBuffer->close();
        currentAudioBuffer->deleteLater();
        currentAudioBuffer = nullptr;
        qDebug() << "Cleaned up audio buffer";
    }

    // 清理临时音频文件
    if (!currentTempAudioFile.isEmpty()) {
        if (QFile::exists(currentTempAudioFile)) {
            QFile::remove(currentTempAudioFile);
            qDebug() << "Removed temporary audio file:" << currentTempAudioFile;
        }
        currentTempAudioFile.clear();
    }

    // 重置播放器源
    mediaPlayer->setSource(QUrl());

    // 重置看门狗计数器
    stuckCount = 0;
    lastPosition = 0;
}

void Widget::checkPlaybackHealth()
{
    // 只在播放状态时检查
    if (mediaPlayer->playbackState() != QMediaPlayer::PlayingState) {
        return;
    }

    qint64 currentPosition = mediaPlayer->position();

    // 如果位置没有变化，可能是卡住了
    if (currentPosition == lastPosition) {
        stuckCount++;
        qDebug() << "Playback might be stuck, count:" << stuckCount;

        // 连续3次（15秒）位置不变，认为卡住了
        if (stuckCount >= 3) {
            qDebug() << "Playback stuck detected, attempting recovery...";

            // 尝试恢复：暂停后继续播放
            mediaPlayer->pause();
            QTimer::singleShot(100, [this]() {
                mediaPlayer->play();
            });

            stuckCount = 0;
        }
    } else {
        // 位置有变化，重置计数器
        stuckCount = 0;
    }

    lastPosition = currentPosition;
}

void Widget::playSong(qint64 id)
{
    if (id <= 0) return;

    // 清理之前的播放资源
    cleanupPreviousPlayback();

    currentPlayingSongId = id; // 更新当前播放的歌曲ID
    currentBvid.clear(); // 清除Bilibili BV号

    // 从播放列表获取当前歌曲信息
    Song currentSong = playlistManager->getCurrentSong();
    if (currentSong.id == id) {
        songNameLabel->setText(currentSong.name);
        // 更新悬浮窗信息
        floatingIsland->setSongInfo(currentSong.name, currentSong.artist, QPixmap());
    } else {
        songNameLabel->setText("加载中...");
    }


    // 重置UI
    originalAlbumArt = QPixmap();
    albumArtLabel->setPixmap(QPixmap());
    flowAnimation->stop(); // 停止流动动画
    currentPalette.clear();
    lyricLabel->setText("歌词加载中...");
    setWidgetStyle(QColor(51, 51, 51));

    // 请求播放链接
    apiManager->getSongUrl(id);

    // 获取歌词和详情
    apiManager->getLyric(id);
    apiManager->getSongDetail(id);

    // 切换到播放详情页
    mainStackedWidget->setCurrentWidget(playerPage);
}

void Widget::playBilibiliVideo(const QString &bvid)
{
    if (bvid.isEmpty()) return;

    // 清理之前的播放资源
    cleanupPreviousPlayback();

    currentBvid = bvid; // 更新当前播放的BV号
    currentPlayingSongId = -1; // 清除网易云音乐ID

    // 从播放列表获取当前歌曲信息
    Song currentSong = playlistManager->getCurrentSong();
    if (currentSong.bvid == bvid) {
        songNameLabel->setText(currentSong.name);
        // 更新悬浮窗信息
        floatingIsland->setSongInfo(currentSong.name, currentSong.artist, QPixmap());
    } else {
        songNameLabel->setText("加载中...");
    }

    // 重置UI
    originalAlbumArt = QPixmap();
    albumArtLabel->setPixmap(QPixmap());
    flowAnimation->stop(); // 停止流动动画
    currentPalette.clear();
    lyricLabel->setText("Bilibili视频 - 无歌词");
    setWidgetStyle(QColor(51, 51, 51));

    // 显示加载动画（隐藏播放按钮，显示加载标签）
    playPauseButton->hide();
    loadingSpinner->start(); // 启动加载动画
    loadingSpinner->show();

    // 获取视频信息（包含cid和封面）
    apiManager->getBilibiliVideoInfo(bvid);

    // 切换到播放详情页
    mainStackedWidget->setCurrentWidget(playerPage);
}

void Widget::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    // 当歌曲播放结束时，自动播放下一首
    if (status == QMediaPlayer::EndOfMedia) {
        currentPlayingSongId = -1; // 播放结束，重置ID
        currentBvid.clear(); // 清除BV号
        playNextSong();
    }
}

void Widget::playNextSong()
{
    if (playlistManager->isEmpty()) return;

    Song nextSong = playlistManager->getNextSong();
    if (nextSong.source == SearchSource::Bilibili && !nextSong.bvid.isEmpty()) {
        playBilibiliVideo(nextSong.bvid);
    } else if (nextSong.id != -1) {
        playSong(nextSong.id);
    }
}

void Widget::playPreviousSong()
{
    if (playlistManager->isEmpty()) return;

    Song prevSong = playlistManager->getPreviousSong();
    if (prevSong.source == SearchSource::Bilibili && !prevSong.bvid.isEmpty()) {
        playBilibiliVideo(prevSong.bvid);
    } else if (prevSong.id != -1) {
        playSong(prevSong.id);
    }
}

void Widget::changePlayMode()
{
    PlaylistManager::PlayMode currentMode = playlistManager->getPlayMode();
    int nextModeIndex = (static_cast<int>(currentMode) + 1) % 3;
    PlaylistManager::PlayMode nextMode = static_cast<PlaylistManager::PlayMode>(nextModeIndex);
    playlistManager->setPlayMode(nextMode);

    switch(nextMode) {
        case PlaylistManager::Sequential:
            playModeButton->setIcon(QIcon(":/icons/loop-list.png"));
            playModeButton->setToolTip("顺序播放");
            break;
        case PlaylistManager::LoopOne:
            playModeButton->setIcon(QIcon(":/icons/loop-one.png"));
            playModeButton->setToolTip("单曲循环");
            break;
        case PlaylistManager::Random:
            playModeButton->setIcon(QIcon(":/icons/shuffle.png"));
            playModeButton->setToolTip("随机播放");
            break;
    }
}

void Widget::updateVolumeIcon(int volume)
{
    if (volume == 0) {
        volumeButton->setIcon(QIcon(":/icons/volume-mute.png"));
    } else if (volume < 30) {
        volumeButton->setIcon(QIcon(":/icons/volume-low.png"));
    } else if (volume < 70) {
        volumeButton->setIcon(QIcon(":/icons/volume-medium.png"));
    } else {
        volumeButton->setIcon(QIcon(":/icons/volume-high.png"));
    }
}

void Widget::onPrevPageButtonClicked()
{
    if (currentPage > 1) {
        currentPage--;
        searchButton->setEnabled(false);
        searchButton->setToolTip("加载中...");
        if (currentSearchSource == SearchSource::NetEase) {
            apiManager->searchSongs(currentSearchKeywords, 15, (currentPage - 1) * 15);
        } else {
            apiManager->searchBilibiliVideos(currentSearchKeywords, currentPage);
        }
    }
}

void Widget::onNextPageButtonClicked()
{
    // 这里的总页数判断依赖于 onSearchFinished 的结果
    currentPage++;
    searchButton->setEnabled(false);
    searchButton->setToolTip("加载中...");
    if (currentSearchSource == SearchSource::NetEase) {
        apiManager->searchSongs(currentSearchKeywords, 15, (currentPage - 1) * 15);
    } else {
        apiManager->searchBilibiliVideos(currentSearchKeywords, currentPage);
    }
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

// 提取多个主色调（苹果音乐风格）
QVector<QColor> Widget::extractPaletteColors(const QPixmap &pixmap, int colorCount)
{
    QVector<QColor> colors;
    if (pixmap.isNull()) {
        colors.append(QColor(51, 51, 51));
        return colors;
    }
    
    // 缩小图片以加快处理速度
    QImage image = pixmap.toImage().scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // 使用区域采样法提取不同区域的主色
    int w = image.width();
    int h = image.height();
    
    // 定义采样区域（左上、右上、中心、左下、右下）
    struct Region { int x1, y1, x2, y2; };
    QVector<Region> regions = {
        {0, 0, w/2, h/2},           // 左上
        {w/2, 0, w, h/2},           // 右上
        {w/4, h/4, w*3/4, h*3/4},   // 中心
        {0, h/2, w/2, h},           // 左下
        {w/2, h/2, w, h}            // 右下
    };
    
    // 计算每个区域的平均颜色
    QVector<QColor> regionColors;
    for (const auto &region : regions) {
        long r = 0, g = 0, b = 0;
        int count = 0;
        
        for (int y = region.y1; y < region.y2 && y < h; ++y) {
            for (int x = region.x1; x < region.x2 && x < w; ++x) {
                QColor c = image.pixelColor(x, y);
                r += c.red();
                g += c.green();
                b += c.blue();
                count++;
            }
        }
        
        if (count > 0) {
            regionColors.append(QColor(r/count, g/count, b/count));
        }
    }
    
    // 选择最亮的颜色作为主色（用于文字等）
    std::sort(regionColors.begin(), regionColors.end(), [](const QColor &a, const QColor &b) {
        return (a.red()*0.299 + a.green()*0.587 + a.blue()*0.114) > 
               (b.red()*0.299 + b.green()*0.587 + b.blue()*0.114);
    });
    
    // 选择最有代表性的颜色（避免太相似的颜色）
    for (const QColor &c : regionColors) {
        bool tooSimilar = false;
        for (const QColor &existing : colors) {
            int dr = qAbs(c.red() - existing.red());
            int dg = qAbs(c.green() - existing.green());
            int db = qAbs(c.blue() - existing.blue());
            if (dr + dg + db < 80) { // 颜色差异阈值
                tooSimilar = true;
                break;
            }
        }
        if (!tooSimilar) {
            colors.append(c);
            if (colors.size() >= colorCount) break;
        }
    }
    
    // 如果颜色不够，用主色生成变体
    if (colors.size() < colorCount && !colors.isEmpty()) {
        QColor base = colors.first();
        while (colors.size() < colorCount) {
            QColor variant = base.darker(120 + colors.size() * 30);
            colors.append(variant);
        }
    }
    
    // 确保至少返回一个颜色
    if (colors.isEmpty()) {
        colors.append(QColor(51, 51, 51));
    }
    
    return colors;
}

// 使用调色板更新背景
void Widget::updateBackgroundWithPalette(const QVector<QColor> &colors)
{
    currentPalette = colors;
    
    if (colors.isEmpty()) {
        setWidgetStyle(QColor(51, 51, 51));
        flowAnimation->stop();
        return;
    }
    
    // 更新流动背景的颜色
    flowingBackground->setColors(colors);
    
    // 启动流动动画
    if (flowAnimation->state() != QAbstractAnimation::Running) {
        flowAnimation->start();
    }
    
    // 使用调色板设置样式
    setWidgetStyleWithPalette(colors);
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
        QLabel#backgroundLabel {
            background-color: transparent;
        }
        QLineEdit {
            background-color: rgba(0, 0, 0, 0.2);
            border: none;
            border-radius: 5px;
            padding: 5px;
            color: %1;
        }
        QComboBox {
            background-color: rgba(0, 0, 0, 0.2);
            border: none;
            border-radius: 5px;
            padding: 5px;
            color: %1;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: %3;
            color: %1;
            selection-background-color: %1;
            selection-color: %2;
            border: none;
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
        "QWidget#mainWidget { background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 %1, stop: 1 %2); }"
    ).arg(color.name(), darkerColor);
    
    this->setStyleSheet(styleSheet + mainWidgetStyle);
}

// 使用调色板设置样式（苹果音乐风格）
void Widget::setWidgetStyleWithPalette(const QVector<QColor> &colors)
{
    if (colors.isEmpty()) {
        setWidgetStyle(QColor(51, 51, 51));
        return;
    }
    
    QColor primaryColor = colors.first();
    currentBackgroundColor = primaryColor;
    
    // 由于模糊背景有暗色遮罩，始终使用浅色文字
    QString foregroundColor = "#FFFFFF";
    QString foregroundColorMuted = "rgba(255, 255, 255, 0.7)";
    
    QString darkerColor = colors.last().darker(150).name();
    
    QString styleSheet = QString(R"(
        QWidget {
            background-color: transparent; 
            color: %1;
            font-family: 'Microsoft YaHei';
        }
        QLabel#backgroundLabel {
            background-color: transparent;
        }
        QLabel#lyricLabel {
            color: %2;
        }
        QLineEdit {
            background-color: rgba(0, 0, 0, 0.35);
            border: none;
            border-radius: 5px;
            padding: 5px;
            color: %1;
        }
        QComboBox {
            background-color: rgba(0, 0, 0, 0.35);
            border: none;
            border-radius: 5px;
            padding: 5px;
            color: %1;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: rgba(30, 30, 30, 0.95);
            color: %1;
            selection-background-color: rgba(255, 255, 255, 0.2);
            selection-color: %1;
            border: none;
            border-radius: 5px;
        }
        QPushButton {
            background-color: rgba(0, 0, 0, 0.35);
            border: none;
            border-radius: 5px;
            padding: 5px 10px;
            color: %1;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.15);
        }
        QPushButton:pressed {
            background-color: rgba(255, 255, 255, 0.25);
        }
        QListWidget {
            background-color: rgba(0, 0, 0, 0.35);
            border: none;
            border-radius: 5px;
        }
        QListWidget::item {
            padding: 10px;
            background-color: transparent;
            border-radius: 5px;
        }
        QListWidget::item:hover {
            background-color: rgba(255, 255, 255, 0.1);
        }
        QListWidget::item:selected {
            background-color: rgba(255, 255, 255, 0.2);
        }
        QSlider::groove:horizontal {
            border: none;
            height: 4px;
            background: rgba(255, 255, 255, 0.2);
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
            background: rgba(255, 255, 255, 0.2);
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
            background-color: rgba(30, 30, 30, 0.95);
            border: none;
            border-radius: 5px;
            padding: 5px;
            color: %1;
        }
    )").arg(foregroundColor, foregroundColorMuted);

    // 背景使用半透明以便看到模糊背景
    QString mainWidgetStyle = QString(
        "QWidget#mainWidget { background-color: rgba(0, 0, 0, 0.1); }"
    );
    
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

void Widget::closeEvent(QCloseEvent *event)
{
    // 忽略默认的关闭事件，而是隐藏窗口并显示托盘图标
    if (this->isVisible()) {
        event->ignore();
        this->hide();
        trayIcon->show();
        trayIcon->showMessage("Melody", "播放器已最小化到托盘");
    } else {
        event->accept();
    }
}

void Widget::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    // 双击托盘图标时显示窗口
    if (reason == QSystemTrayIcon::DoubleClick) {
        this->showNormal();
        this->activateWindow();
        trayIcon->hide();
    }
}

void Widget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // 调整流动背景大小
    flowingBackground->setGeometry(0, 0, this->width(), this->height());
    
    if (!originalAlbumArt.isNull())
    {
        int size = qMin(this->width(), this->height()) * 0.6;
        albumArtLabel->setPixmap(originalAlbumArt.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}
