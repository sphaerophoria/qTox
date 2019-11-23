import QtQuick 2.13
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.13
import org.qtox.status 1.0

Item {
    width: parent.width
    height: 20

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Text {
            text: modelData.name
            font.bold: true
            renderType: Text.NativeRendering
            color: "white"
        }

        Text {
            Layout.alignment: Qt.AlignRight
            text: "" + modelData.peersCount + " user(s) in chat"
            font.bold: true
            renderType: Text.NativeRendering
            color: "lightgrey"
        }

        StatusPic {
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignRight
            toxStatus: Status.Online
        }
    }

    MouseArea {
        anchors.fill:parent
        onClicked: list.currentIndex = index
    }
}
