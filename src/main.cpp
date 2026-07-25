#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QSurfaceFormat>
#include <QQuickWindow>
#include <QFontDatabase>
#include <QFont>
#include "core/logger.h"
#include "core/musiccontroller.h"

int main(int argc, char *argv[])
{
    // Enable window alpha buffer for transparent/rounded corners (RHI backends like D3D11/Vulkan/OpenGL)
    QQuickWindow::setDefaultAlphaBuffer(true);

    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    // Use Basic style so QML controls support full custom styling
    QQuickStyle::setStyle("Basic");

    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/logo.png"));
    app.setApplicationName("Melody");
    app.setOrganizationName("Melody");

    // 日志系统必须在 applicationName/organizationName 设置之后初始化，
    // 以便 QStandardPaths::AppDataLocation 解析到正确的目录。
    initLogging();

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
    closeLogging();
    return ret;
}
