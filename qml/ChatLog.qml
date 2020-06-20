import QtQuick 2.13
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

Item {
    id: root
    property var model

    ListView {
        property int senderWidth
        property int timestampWidth

        id: listView
        anchors.fill: parent
        anchors.rightMargin: listViewScrollBar.width
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        highlightMoveDuration: 1
        highlightMoveVelocity: -1
        spacing: 5

        model: root.model
        delegate: ColumnLayout {
            property var prevIndex: index > 0 ? chatModel.index(index - 1, 0) : undefined
            anchors.left: parent ? parent.left : undefined
            anchors.right: parent ? parent.right : undefined

            TextEdit {
                Layout.leftMargin: listView.senderWidth
                Layout.topMargin: 10
                Layout.bottomMargin: 10
                readOnly: true
                selectByMouse: true
                text: Qt.formatDate(timestamp, "<b>yyyy-MM-dd</b>")
                textFormat: TextEdit.RichText
                visible: {
                    if (prevIndex) {
                        var prevDate = chatModel.data(prevIndex, 0x103)
                        return timestamp.getDate() != prevDate.getDate() ||
                            timestamp.getFullYear() != prevDate.getFullYear() ||
                            timestamp.getMonth() != prevDate.getMonth()
                    }
                    return true
                }
            }
            RowLayout {
                spacing: 5
                Layout.fillWidth: true
                Layout.fillHeight: true

                TextEdit {
                    readOnly: true
                    selectByMouse: true
                    text: {
                        if (index > 0)  {
                            var prevIndex = chatModel.index(index - 1, 0)
                            var prevSender = chatModel.data(prevIndex, 0x102)
                            if (prevSender == sender) {
                                return ""
                            }
                        }

                        return displayName ? displayName : ""
                    }
                    clip: true

                    Layout.preferredWidth: listView.senderWidth

                    Component.onCompleted: {
                        listView.senderWidth = Math.max(listView.senderWidth, contentWidth)
                    }
                }
                TextEdit {
                    Layout.fillWidth: true

                    readOnly: true
                    selectByMouse: true
                    text: message
                    textFormat: TextEdit.RichText
                    wrapMode: Text.Wrap
                }
                TextEdit {
                    readOnly: true
                    selectByMouse: true
                    text: {
                        if (messagePending == true) {
                            return "..."
                        }
                        return Qt.formatTime(timestamp, "hh:mm:ss")
                    }
                    clip: true
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: listView.timestampWidth

                    Component.onCompleted: {
                        listView.timestampWidth = Math.max(listView.timestampWidth, contentWidth)
                    }
                }
            }
        }

        Component.onCompleted: {
            listView.currentIndex = count - 1
        }

        onCountChanged: {
            listView.currentIndex = count - 1
        }

        ScrollBar.vertical: listViewScrollBar
    }

    ScrollBar {
        id: listViewScrollBar
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
