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
    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
        onWidthChanged: {
            console.log("Scrollview width: " + width)
        }

        ListView {
            property int senderWidth
            property int timestampWidth

            id: tableView

            anchors.fill: parent
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            model: chatModel
            delegate: RowLayout {
                anchors.right: parent.right
                anchors.left: parent.left
                spacing: 5

                Text {
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

                    Layout.preferredWidth: {
                        tableView.senderWidth = Math.max(contentWidth, tableView.senderWidth)
                        return Math.min(tableView.senderWidth, 100)
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: message
                    wrapMode: Text.Wrap
                }
                Text {
                    text: Qt.formatTime(timestamp, "hh:mm:ss")
                    clip: true
                    Layout.alignment: Qt.AlignRight
                    Layout.preferredWidth: {
                        tableView.timestampWidth = Math.max(contentWidth, tableView.timestampWidth)
                        return tableView.timestampWidth
                    }
                }
            }
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
