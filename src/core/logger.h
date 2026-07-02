#ifndef LOGGER_H
#define LOGGER_H

#include <QLoggingCategory>

// 日志分类：按核心模块划分，便于通过 QT_LOGGING_RULES 或代码按需过滤。
//   core.api     —— 网络/API 层（ApiManager）
//   core.player  —— 播放控制层（MusicController）
//   core.playlist—— 播放列表数据层（PlaylistManager）
Q_DECLARE_LOGGING_CATEGORY(logApi)
Q_DECLARE_LOGGING_CATEGORY(logPlayer)
Q_DECLARE_LOGGING_CATEGORY(logPlaylist)

// 安装全局消息处理器，日志写入系统标准目录：
//   Windows: %APPDATA%/Melody/melody/logs/melody_<yyyyMMdd>.log
// 必须在 QGuiApplication 设置 applicationName/organizationName 之后调用。
// 返回日志文件所在目录，失败时返回空 QString。
QString initLogging();

// 关闭并释放日志文件。可在 app.exec() 返回后调用。
void closeLogging();

#endif // LOGGER_H
