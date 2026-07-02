#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QSurfaceFormat>
#include <QFile>
#include <QTextStream>
#include <QFontDatabase>
#include <QFont>
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

    // Font loading: JetBrains Mono (base) + HarmonyOS Sans SC (CJK fallback) + codicons.
    // Reference: ImZiv (ImGui) uses ImFont merge mode; Qt equivalent is insertSubstitution.
    int idJetBrains = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono.ttf");
    int idHarmony   = QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_SC_Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/codicons.ttf");

    const QStringList jetbrainsFamilies = QFontDatabase::applicationFontFamilies(idJetBrains);
    const QStringList harmonyFamilies   = QFontDatabase::applicationFontFamilies(idHarmony);
    if (!jetbrainsFamilies.isEmpty()) {
        QFont baseFont(jetbrainsFamilies.first());
        baseFont.setPixelSize(13);
        app.setFont(baseFont);
        if (!harmonyFamilies.isEmpty()) {
            // CJK glyphs missing in JetBrains Mono fall back to HarmonyOS Sans SC.
            QFont::insertSubstitution(jetbrainsFamilies.first(), harmonyFamilies.first());
        }
    }

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

