#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QSurfaceFormat>
#include <QFile>
#include <QTextStream>
#include "core/musiccontroller.h"

static QFile *g_logFile = nullptr;
static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(type); Q_UNUSED(ctx);
    if (g_logFile && g_logFile->isOpen()) {
        QTextStream(g_logFile) << msg << "\n";
        g_logFile->flush();
    }
}

int main(int argc, char *argv[])
{
    g_logFile = new QFile("melody_debug.log");
    g_logFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    qInstallMessageHandler(messageHandler);

    // Enable window alpha buffer for transparent/rounded corners
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    // Use Basic style so QML controls support full custom styling
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/logo.png"));
    app.setApplicationName("Melody");
    app.setOrganizationName("Melody");

    MusicController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.addImportPath("qrc:/");

    const QUrl url(QStringLiteral("qrc:/Melody/src/qml/Main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated,
        &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection
    );
    engine.load(url);

    int ret = app.exec();
    if (g_logFile) { g_logFile->close(); delete g_logFile; }
    return ret;
}

