import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Melody

// Settings page: storage maintenance (cache / log cleanup)
Item {
    id: root

    property int cacheBytes: 0
    property int logBytes: 0
    // Brief "cleaned" feedback per row: "" | "cache" | "logs"
    property string justCleaned: ""

    function refresh() {
        cacheBytes = controller.cacheSizeBytes()
        logBytes = controller.logSizeBytes()
    }

    function fmtSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KB"
        return (bytes / 1024 / 1024).toFixed(1) + " MB"
    }

    Component.onCompleted: refresh()

    Connections {
        target: controller
        function onCacheInfoChanged() { root.refresh() }
    }

    Timer {
        id: cleanedFeedbackTimer
        interval: 1500
        onTriggered: root.justCleaned = ""
    }

    ColumnLayout {
        anchors { fill: parent; margins: Theme.spaceLg; topMargin: 10 }
        spacing: Theme.spaceMd

        // Back button + title row
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            IconBtn {
                source: "qrc:/icons/arrow-left.svg"
                width: 32; height: 32; radius: Theme.radiusSm
                onClicked: root.StackView.view.pop()
            }
            Text {
                text: "设置"
                color: Theme.textPrimary
                font { pixelSize: 16; bold: true; family: Theme.fontMain }
            }
            Item { Layout.fillWidth: true }
        }

        // ── Storage section ───────────────────────────────────
        Text {
            text: "存储"
            color: Theme.textSecondary
            font { pixelSize: 12; family: Theme.fontMain }
            Layout.topMargin: Theme.spaceSm
        }

        // Cache row
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            radius: Theme.radiusMd
            color: Theme.overlay06

            RowLayout {
                anchors { fill: parent; leftMargin: Theme.spaceMd; rightMargin: Theme.spaceMd }
                spacing: Theme.spaceSm

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "缓存(封面与音频)"
                        color: Theme.textPrimary
                        font { pixelSize: 13; family: Theme.fontMain }
                    }
                    Text {
                        text: root.justCleaned === "cache" ? "已清理 ✓" : root.fmtSize(root.cacheBytes)
                        color: root.justCleaned === "cache" ? Theme.accentLight : Theme.textTertiary
                        font { pixelSize: 11; family: Theme.fontMain }
                        Behavior on color { ColorAnimation { duration: Theme.durNormal } }
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 28
                    radius: Theme.radiusSm
                    color: clearCacheArea.pressed ? Theme.overlay18
                         : clearCacheArea.containsMouse ? Theme.overlay10
                         : Theme.overlay06
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }

                    Text {
                        anchors.centerIn: parent
                        text: "清理"
                        color: Theme.textPrimary
                        opacity: 0.85
                        font { pixelSize: 12; family: Theme.fontMain }
                    }
                    MouseArea {
                        id: clearCacheArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            controller.clearCache()
                            root.justCleaned = "cache"
                            cleanedFeedbackTimer.restart()
                        }
                    }
                }
            }
        }

        // Logs row
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            radius: Theme.radiusMd
            color: Theme.overlay06

            RowLayout {
                anchors { fill: parent; leftMargin: Theme.spaceMd; rightMargin: Theme.spaceMd }
                spacing: Theme.spaceSm

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "日志文件"
                        color: Theme.textPrimary
                        font { pixelSize: 13; family: Theme.fontMain }
                    }
                    Text {
                        text: root.justCleaned === "logs" ? "已清理 ✓(保留今日)" : root.fmtSize(root.logBytes)
                        color: root.justCleaned === "logs" ? Theme.accentLight : Theme.textTertiary
                        font { pixelSize: 11; family: Theme.fontMain }
                        Behavior on color { ColorAnimation { duration: Theme.durNormal } }
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 28
                    radius: Theme.radiusSm
                    color: clearLogsArea.pressed ? Theme.overlay18
                         : clearLogsArea.containsMouse ? Theme.overlay10
                         : Theme.overlay06
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }

                    Text {
                        anchors.centerIn: parent
                        text: "清理"
                        color: Theme.textPrimary
                        opacity: 0.85
                        font { pixelSize: 12; family: Theme.fontMain }
                    }
                    MouseArea {
                        id: clearLogsArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            controller.clearLogs()
                            root.justCleaned = "logs"
                            cleanedFeedbackTimer.restart()
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
