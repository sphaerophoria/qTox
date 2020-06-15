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
    Rectangle {
        Layout.fillHeight: true
        Layout.fillWidth: true
        ScrollView {
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            anchors.fill:parent

            ListView {
                anchors.fill: parent
                boundsBehavior: Flickable.StopAtBounds
                model: 20
                delegate: ItemDelegate {
                    text: "Item " + index
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
