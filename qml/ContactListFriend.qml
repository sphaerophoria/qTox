import QtQuick 2.13
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.13

Item {
    width: parent.width
    height: 20

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Text {
            text: modelData.contact.name
            font.bold: true
            renderType: Text.NativeRendering
            color: "white"
        }

        Text {
            Layout.alignment: Qt.AlignRight
            text: modelData.contact.statusMessage
            font.bold: true
            renderType: Text.NativeRendering
            color: "lightgrey"
        }

        StatusPic {
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignRight
            toxStatus: modelData.contact.status
        }
    }

    MouseArea {
        anchors.fill:parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            if (mouse.button === Qt.RightButton) {
                contextMenu.popup()
            } else if (mouse.button === Qt.LeftButton) {
                list.currentIndex = index
            }
        }

        Menu {
            id: contextMenu

            MenuItem { text: qsTr("Invite to group") }
            MenuItem { text: qsTr("Move to circle") }
            MenuItem { text: qsTr("Set alias...") }
            MenuItem { text: qsTr("Autoaccept files from this friend") }
            MenuItem { text: qsTr("Remove friend") }
            MenuItem { text: qsTr("Show details") }
        }
    }
}
