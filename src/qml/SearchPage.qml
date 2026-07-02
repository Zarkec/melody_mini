import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// Search page: search bar + source selector + result list + pagination
Item {
    id: root
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
                           ? Qt.rgba(1,1,1,0.18)
                           : Qt.rgba(1,1,1,0.06)
                    Behavior on color { ColorAnimation { duration: 180 } }

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: "white"
                        font { pixelSize: 12; family: "Segoe UI" }
                        opacity: controller.searchSource === index ? 1.0 : 0.5
                        Behavior on opacity { NumberAnimation { duration: 180 } }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: controller.searchSource = index
                    }
                }
            }
        }

        // ── Search input row ──────────────────────────────────
        RowLayout {
            spacing: 8
            Layout.fillWidth: true
            Layout.preferredHeight: 42

            // Input field container
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 42
                height: 42
                radius: 12
                color: Qt.rgba(1,1,1,0.09)
                border { color: searchField.activeFocus ? Qt.rgba(1,1,1,0.25) : "transparent"; width: 1 }
                Behavior on border.color { ColorAnimation { duration: 150 } }

                RowLayout {
                    anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                    spacing: 8

                    Image {
                        source: "qrc:/icons/search.png"
                        sourceSize.width: 15
                        sourceSize.height: 15
                        Layout.alignment: Qt.AlignVCenter
                        opacity: 0.45
                    }

                    TextInput {
                        id: searchField
                        Layout.fillWidth: true
                        height: parent.height
                        color: "white"
                        selectionColor: Qt.rgba(0.6, 0.5, 1, 0.4)
                        font { pixelSize: 13; family: "Segoe UI" }
                        clip: true
                        verticalAlignment: TextInput.AlignVCenter
                        Keys.onReturnPressed: doSearch()

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
                        color: Qt.rgba(1,1,1,0.4)
                        font.pixelSize: 11
                        Layout.alignment: Qt.AlignVCenter
                        MouseArea {
                            anchors.fill: parent
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
                radius: 12
                color: searchBtnArea.pressed ? Qt.rgba(0.6,0.5,1,0.30) : Qt.rgba(1,1,1,0.09)
                Behavior on color { ColorAnimation { duration: 120 } }

                Image {
                    anchors.centerIn: parent
                    source: "qrc:/icons/search.png"
                    sourceSize.width: 18
                    sourceSize.height: 18
                    opacity: controller.searchLoading ? 0.4 : 0.85
                }

                MouseArea {
                    id: searchBtnArea
                    anchors.fill: parent
                    enabled: !controller.searchLoading
                    onClicked: doSearch()
                }
            }
        }

        // ── Status / error message ────────────────────────────
        Text {
            visible: controller.statusMessage !== ""
            text: controller.statusMessage
            color: controller.statusIsError ? "#ff6b6b" : Qt.rgba(1,1,1,0.55)
            font { pixelSize: 12; family: "Segoe UI" }
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
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
                color: ma.containsMouse ? Qt.rgba(1,1,1,0.10) : "transparent"
                Behavior on color { ColorAnimation { duration: 100 } }

                Row {
                    anchors { fill: parent; leftMargin: 14; rightMargin: 14 }
                    spacing: 10

                    // Source badge
                    Rectangle {
                        width: 18; height: 18; radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        color: modelData.source === 1 ? "#fb7299" : "#e05555"
                        Text {
                            anchors.centerIn: parent
                            text: modelData.source === 1 ? "B" : "N"
                            color: "white"
                            font { pixelSize: 9; bold: true }
                        }
                    }

                    Column {
                        width: parent.width - 18 - 10 - 20 - 10
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3

                        Text {
                            width: parent.width
                            text: modelData.title || ""
                            color: "white"
                            font { pixelSize: 13; family: "Segoe UI" }
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: modelData.artist || ""
                            color: Qt.rgba(1,1,1,0.45)
                            font { pixelSize: 11; family: "Segoe UI" }
                            elide: Text.ElideRight
                        }
                    }

                    Image {
                        id: playArrow
                        source: "qrc:/icons/play.png"
                        width: 14; height: 14
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: ma.containsMouse ? 0.55 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }
                }

                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.songClicked(index)
                }
            }

            // ── Loading overlay ──
            Item {
                visible: controller.searchLoading
                anchors.fill: parent
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
                        font { pixelSize: 12; family: "Segoe UI" }
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }

            // ── Empty state ──
            Column {
                visible: !controller.searchLoading && controller.searchResults.length === 0
                anchors.centerIn: parent
                spacing: 12
                Text {
                    text: "🎵"
                    font.pixelSize: 38
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "搜索你喜欢的音乐"
                    color: Qt.rgba(1,1,1,0.30)
                    font { pixelSize: 13; family: "Segoe UI" }
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
                source: "qrc:/icons/previous-page.png"
                width: 36; height: 36; radius: 8
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
                color: Qt.rgba(1,1,1,0.50)
                font { pixelSize: 12; family: "Segoe UI" }
            }

            IconBtn {
                source: "qrc:/icons/next-page.png"
                width: 36; height: 36; radius: 8
                enabled: controller.currentPage < controller.totalPages && !controller.searchLoading
                onClicked: controller.nextPage()
            }
        }
    }

    function doSearch() {
        searchField.focus = true
        controller.search(searchField.text)
    }
}
