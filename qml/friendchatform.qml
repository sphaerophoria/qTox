import QtQuick 2.13
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15


ColumnLayout {
    // chatform header
    Text {
        text: "header"
    }
    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: 'lightgrey'
    }
    // Chatform
    Item {
        Layout.fillWidth: true
        Layout.fillHeight: true
        ListView {
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                rightMargin: listViewScrollBar.width

            }

            property int senderWidth
            property int timestampWidth

            id: tableView

            boundsBehavior: Flickable.StopAtBounds
            clip: true

            highlightMoveDuration: 1
            highlightMoveVelocity: -1
            spacing: 5

            model: chatModel
            delegate: RowLayout {
                anchors.left: {
                    if (parent) {
                        return parent.left
                    }
                }
                anchors.right: {
                    if (parent) {
                        return parent.right
                    }
                }
                spacing: 5

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

                        return displayName
                    }
                    clip: true

                    Layout.preferredWidth: tableView.senderWidth

                    Component.onCompleted: {
                        tableView.senderWidth = Math.max(tableView.senderWidth, contentWidth)
                    }
                }
                TextEdit {
                    readOnly: true
                    selectByMouse: true
                    Layout.fillWidth: true
                    text: message
                    wrapMode: Text.Wrap
                }
                TextEdit {
                    readOnly: true
                    selectByMouse: true
                    text: Qt.formatTime(timestamp, "hh:mm:ss")
                    clip: true
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: tableView.timestampWidth

                    Component.onCompleted: {
                        tableView.timestampWidth = Math.max(tableView.timestampWidth, contentWidth)
                    }
                }
            }

            Component.onCompleted: {
                tableView.currentIndex = count - 1
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
    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: 'lightgrey'
    }
    // Text area
    RowLayout {
        RowLayout {
            Layout.fillHeight: false

            TextArea {
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: TextEdit.WordWrap
                verticalAlignment: TextEdit.AlignTop
                text: "asdfasdf"
            }
            ColumnLayout {
                Layout.alignment: Qt.AlignRight
                Button {
                    icon.source: "../themes/default/chatForm/emoteButton.svg"
                }
                Button {
                    icon.source: "../themes/default/chatForm/fileButton.svg"
                }
            }
        }
        Button {
            Layout.alignment: Qt.AlignRight
            icon.source: "../themes/default/chatForm/sendButton.svg"
        }
    }
}
