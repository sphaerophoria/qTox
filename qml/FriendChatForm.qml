import QtQuick 2.13
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15


ColumnLayout {
    id: root
    signal reloadUi();
    signal messageSent(string message);

    Keys.onPressed: {
        if (event.key == Qt.Key_F5) {
            reloadUi()
        }
    }

    // chatform header
    Text {
        text: contact.name
    }
    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: 'lightgrey'
    }
    ChatLog {
        Layout.fillWidth: true
        Layout.fillHeight: true

        model: chatModel
    }
    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: 'lightgrey'
    }
    // Text area
    RowLayout {
        Layout.maximumHeight: 100
        RowLayout {
            Layout.fillHeight: false

            TextArea {
                property var sendMessage: function() {
                    root.messageSent(text)
                    text = ""
                }

                id: chatTextArea
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: TextEdit.WordWrap
                verticalAlignment: TextEdit.AlignTop
                placeholderText: "Type your message here..."

                Keys.onPressed: {
                    if (event.key == Qt.Key_Return && event.modifiers == Qt.NoModifier) {
                        event.accepted = true
                        sendMessage()
                    }
                }
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
            Layout.fillHeight: true
            icon.source: "../themes/default/chatForm/sendButton.svg"
            onPressed: {
                chatTextArea.sendMessage()
            }
        }
    }
}
