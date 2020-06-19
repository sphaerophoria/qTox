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
        text: "header"
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
        RowLayout {
            Layout.fillHeight: false

            TextArea {
                Layout.alignment: Qt.AlignLeft
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: TextEdit.WordWrap
                verticalAlignment: TextEdit.AlignTop
                placeholderText: "Type your message here..."

                Keys.onPressed: {
                    console.log(event.modifiers)
                    if (event.key == Qt.Key_Return && event.modifiers == Qt.NoModifier) {
                        event.accepted = true
                        root.messageSent(text)
                        text = ""
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
            icon.source: "../themes/default/chatForm/sendButton.svg"
        }
    }
}
