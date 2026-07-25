#include "logger.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QMessageLogContext>
#include <QCoreApplication>

Q_LOGGING_CATEGORY(logApi, "core.api")
Q_LOGGING_CATEGORY(logPlayer, "core.player")
Q_LOGGING_CATEGORY(logPlaylist, "core.playlist")

static QFile *g_logFile = nullptr;

static QString levelString(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:    return QStringLiteral("DEBUG");
    case QtInfoMsg:     return QStringLiteral("INFO ");
    case QtWarningMsg:  return QStringLiteral("WARN ");
    case QtCriticalMsg: return QStringLiteral("ERROR");
    case QtFatalMsg:    return QStringLiteral("FATAL");
    }
    return QStringLiteral("?????");
}

static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    // 时间戳 [yyyy-MM-dd hh:mm:ss.zzz]
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));

    // 分类（默认分类返回 "default"，显示为空更清晰）
    const QString category = ctx.category ? QString::fromLatin1(ctx.category) : QStringLiteral("default");

    // 源码位置（文件:行号），Release 下 ctx.file 可能为空
    QString location;
    if (ctx.file && ctx.line > 0) {
        // 仅取文件名，避免绝对路径污染日志
        QString file = QString::fromLatin1(ctx.file);
        int slash = qMax(file.lastIndexOf(QLatin1Char('/')), file.lastIndexOf(QLatin1Char('\\')));
        if (slash >= 0)
            file = file.mid(slash + 1);
        location = QStringLiteral(" [%1:%2]").arg(file).arg(ctx.line);
    }

    // 完整一行：时间 | 级别 | 分类 | 消息 [文件:行号]
    const QString line = QStringLiteral("%1 | %2 | %3 | %4%5\n")
                             .arg(ts, levelString(type), category, msg, location);

    // 写入文件（Qt6 QTextStream 默认 UTF-8）
    if (g_logFile && g_logFile->isOpen()) {
        QTextStream out(g_logFile);
        out << line;
        out.flush();
    }

    // Warning 及以上仍同步输出到 stderr，便于开发期实时观察
    if (type >= QtWarningMsg) {
        QTextStream(stderr) << line;
    }

    if (type == QtFatalMsg)
        abort();
}

QString initLogging()
{
    // 使用系统标准应用数据目录，避免依赖当前工作目录
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QCoreApplication::applicationDirPath();
    dir += QStringLiteral("/logs");
    QDir().mkpath(dir);

    const QString date = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd"));
    const QString path = dir + QStringLiteral("/melody_%1.log").arg(date);

    g_logFile = new QFile(path);
    // Append：同一天多次启动追加到同一文件；如需每次覆盖可改为 Truncate。
    if (!g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        // 打开失败：回退到可执行目录旁，避免日志完全丢失
        delete g_logFile;
        g_logFile = nullptr;
        return QString();
    }

    qInstallMessageHandler(messageHandler);

    qInfo("=========== Melody starting ===========");
    qInfo("Log file: %s", qPrintable(path));

    return dir;
}

void closeLogging()
{
    if (g_logFile) {
        if (g_logFile->isOpen()) {
            QTextStream(g_logFile) << QStringLiteral("=========== Melody exiting ===========\n");
            g_logFile->flush();
            g_logFile->close();
        }
        delete g_logFile;
        g_logFile = nullptr;
    }
}
