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
        id: rectangle
        ScrollView {
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn
            anchors.fill: parent

            TableView {
                property int nameWidth;
                property int dateWidth;

                id: tableView

                anchors.fill: parent
                boundsBehavior: Flickable.StopAtBounds
                columnSpacing: 5
                columnWidthProvider: function (column) {
                    if (column == 0) {
                        return Math.min(tableView.nameWidth, 50)
                    }
                    else if (column == 1) {
                        return width - columnWidthProvider(0) - columnWidthProvider(2) - columnSpacing * 2
                    }
                    else if (column == 2) {
                        return Math.min(tableView.dateWidth, 50)
                    }
                }
                onWidthChanged: forceLayout()

                Component.onCompleted: {
                    nameWidth = 20
                    dateWidth = 20
                    forceLayout()
                }

                model: chatModel
                delegate: Text {
                    text: display
                    clip: true

                    Component.onCompleted: {
                        if (column == 0) {
                            tableView.nameWidth = Math.max(contentWidth, tableView.nameWidth)
                        }
                        else if (column == 2) {
                            tableView.dateWidth = Math.max(contentWidth, tableView.dateWidth)
                        }
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
