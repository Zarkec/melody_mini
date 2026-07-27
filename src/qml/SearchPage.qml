import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Melody

// Search page: search bar + source selector + result list + pagination
Item {
    id: root
    property bool syncingSearchText: false
    // 按搜索源缓存输入框草稿。之前未声明,onTextChanged 里 root.searchDrafts[...] = text
    // 抛 TypeError(刷大量告警),且"切源恢复未提交输入"的功能完全失效。
    property var searchDrafts: ({})
    signal songClicked(int index)

    ColumnLayout {
        anchors {
            fill: parent
            leftMargin: 14
            rightMargin: 14
            topMargin: 14
            bottomMargin: 86
        }
        spacing: 10

        // ── Source selector tabs ──────────────────────────────
        Row {
            spacing: 6
            Layout.fillWidth: true
            height: 34

            Repeater {
                model: ["网易云音乐", "Bilibili"]
                delegate: Rectangle {
                    width: (root.width - 28 - 6) / 2
                    height: 34
                    radius: 10
                    color: controller.searchSource === index
                           ? Theme.overlay18
                           : Theme.overlay06
                    Behavior on color { ColorAnimation { duration: 180; easing.type: Easing.OutCubic } }

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: Theme.textPrimary
                        font { pixelSize: 12; family: Theme.fontMain }
                        opacity: controller.searchSource === index ? 1.0 : 0.5
                        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: controller.searchSource = index
                    }
                }
            }
        }

        // ── Search input row ──────────────────────────────────
        RowLayout {
            spacing: Theme.spaceSm
            Layout.fillWidth: true
            Layout.preferredHeight: 42

            // Input field container
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                height: 42
                radius: Theme.radiusMd
                color: Theme.overlay09
                border { color: searchField.activeFocus ? Qt.rgba(1,1,1,0.25) : "transparent"; width: 1 }
                Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

                RowLayout {
                    anchors { fill: parent; leftMargin: Theme.spaceMd; rightMargin: Theme.spaceMd }
                    spacing: Theme.spaceSm

                    Image {
                        source: "qrc:/icons/search.svg"
                        sourceSize.width: 15
                        sourceSize.height: 15
                        Layout.alignment: Qt.AlignVCenter
                        opacity: 0.45
                    }

                    TextInput {
                        id: searchField
                        Layout.fillWidth: true
                        height: parent.height
                        color: Theme.textPrimary
                        selectionColor: Qt.rgba(0.7, 0.7, 0.7, 0.4)
                        font { pixelSize: 13; family: Theme.fontMain }
                        clip: true
                        verticalAlignment: TextInput.AlignVCenter
                        Keys.onReturnPressed: doSearch()
                        Component.onCompleted: setSearchText(controller.currentKeywords)
                        onTextChanged: {
                            if (!root.syncingSearchText)
                                root.searchDrafts[controller.searchSource] = text
                        }

                        Connections {
                            target: controller
                            function onCurrentKeywordsChanged() { setSearchText(controller.currentKeywords) }
                        }

                        Text {
                            visible: !searchField.text && !searchField.activeFocus
                            text: controller.searchSource === 0
                                  ? "输入歌名或歌手…"
                                  : "输入 Bilibili 视频关键词…"
                            color: Qt.rgba(1,1,1,0.28)
                            font: searchField.font
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    // Clear ✕
                    Text {
                        id: clearBtn
                        visible: searchField.text.length > 0
                        text: "✕"
                        color: clearHover.containsMouse ? Theme.errorRed : Qt.rgba(1,1,1,0.4)
                        font { pixelSize: 16; family: Theme.fontMain }
                        Layout.alignment: Qt.AlignVCenter
                        Behavior on color { ColorAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }
                        MouseArea {
                            id: clearHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: searchField.text = ""
                        }
                    }
                }
            }

            // Search icon button
            Rectangle {
                width: 42; height: 42
                Layout.preferredWidth: 42
                Layout.preferredHeight: 42
                radius: Theme.radiusMd
                color: searchBtnArea.pressed ? Qt.rgba(0.7,0.7,0.7,0.30) : Theme.overlay09
                Behavior on color { ColorAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }
                scale: searchBtnArea.pressed ? 0.96 : 1.0
                Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/icons/search.svg"
                    sourceSize.width: 18
                    sourceSize.height: 18
                    opacity: controller.searchLoading ? 0.4 : 0.85
                }

                MouseArea {
                    id: searchBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    enabled: !controller.searchLoading
                    onClicked: doSearch()
                }
            }
        }

        // ── Status / error message ────────────────────────────
        // Fixed line height + opacity-only animation so the result list
        // does not jump when status appears/disappears.
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 18
            opacity: controller.statusMessage !== "" ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }

            Text {
                anchors.fill: parent
                visible: controller.statusMessage !== ""
                text: controller.statusMessage
                color: controller.statusIsError ? Theme.errorRed : Theme.textSecondary
                font { pixelSize: 12; family: Theme.fontMain }
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
            }
        }

        // ── Result list ───────────────────────────────────────
        ListView {
            id: resultList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 3
            model: controller.searchResults

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                contentItem: Rectangle {
                    implicitWidth: 4
                    radius: 2
                    color: Qt.rgba(1,1,1,0.22)
                }
            }

            delegate: Rectangle {
                id: delegateRoot
                width: resultList.width
                height: 60
                radius: 10
                color: ma.containsMouse ? Theme.overlay10 : "transparent"
                Behavior on color { ColorAnimation { duration: 100; easing.type: Easing.OutCubic } }

                Row {
                    anchors { fill: parent; leftMargin: 14; rightMargin: 14 }
                    spacing: 10

                    // Source badge
                    Rectangle {
                        width: 18; height: 18; radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: modelData.source === 1 ? Theme.badgeBilibili : Theme.badgeNetease
                        Text {
                            anchors.centerIn: parent
                            text: modelData.source === 1 ? "B" : "N"
                            color: Theme.textPrimary
                            font { pixelSize: 9; bold: true; family: Theme.fontMain }
                        }
                    }

                    Column {
                        width: parent.width - 18 - 10 - 20 - 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3

                        Text {
                            width: parent.width
                            text: modelData.title || ""
                            color: Theme.textPrimary
                            font { pixelSize: 13; family: Theme.fontMain }
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: modelData.artist || ""
                            color: Theme.textTertiary
                            font { pixelSize: 11; family: Theme.fontMain }
                            elide: Text.ElideRight
                        }
                    }

                    Image {
                        id: playArrow
                        source: "qrc:/icons/play.svg"
                        width: 14; height: 14
                        sourceSize.width: 28
                        sourceSize.height: 28
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: ma.containsMouse ? 0.55 : 0
                        Behavior on opacity { NumberAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }
                    }
                }

                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.songClicked(index)
                }
            }

            // ── Loading overlay ──
            Item {
                visible: controller.searchLoading
                anchors.fill: parent
                opacity: controller.searchLoading ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }
                Column {
                    anchors.centerIn: parent
                    spacing: 14
                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: controller.searchLoading
                        width: 36; height: 36
                    }
                    Text {
                        text: "搜索中…"
                        color: Qt.rgba(1,1,1,0.4)
                        font { pixelSize: 12; family: Theme.fontMain }
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            // ── Empty state ──
            Column {
                visible: !controller.searchLoading && controller.searchResults.length === 0
                anchors.centerIn: parent
                spacing: Theme.spaceMd
                opacity: (!controller.searchLoading && controller.searchResults.length === 0) ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }
                Text {
                    text: "🎵"
                    font { pixelSize: 38; family: Theme.fontMain }
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "搜索你喜欢的音乐"
                    color: Qt.rgba(1,1,1,0.30)
                    font { pixelSize: 13; family: Theme.fontMain }
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // ── Pagination ────────────────────────────────────────
        Row {
            visible: controller.totalPages > 1
            Layout.fillWidth: true
            spacing: 0
            height: 36

            IconBtn {
                source: "qrc:/icons/chevron-left.svg"
                width: 36; height: 36; radius: Theme.radiusSm
                enabled: controller.currentPage > 1 && !controller.searchLoading
                onClicked: controller.prevPage()
            }

            Text {
                width: root.width - 28 - 36*2
                height: 36
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: controller.totalPages > 0
                    ? ("第 " + controller.currentPage + " / " + controller.totalPages + " 页")
                    : ""
                color: Theme.textSecondary
                font { pixelSize: 12; family: Theme.fontMain }
            }

            IconBtn {
                source: "qrc:/icons/chevron-right.svg"
                width: 36; height: 36; radius: Theme.radiusSm
                enabled: controller.currentPage < controller.totalPages && !controller.searchLoading
                onClicked: controller.nextPage()
            }
        }
    }

    function setSearchText(text) {
        root.syncingSearchText = true
        searchField.text = text || ""
        root.syncingSearchText = false
    }

    function doSearch() {
        searchField.focus = true
        controller.search(searchField.text)
    }
}
