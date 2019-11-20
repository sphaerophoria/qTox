import QtQuick 2.13
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.13
import org.qtox.status 1.0

Item {
    id: root
    signal friendSelected(variant f)

    ListView {
        id: list

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds

        model: friendListModel

        delegate: Item {
            width: parent.width
            height: 20

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Text {
                    text: model.name
                    font.bold: true
                    renderType: Text.NativeRendering
                    color: "white"
                }

                Text {
                    Layout.alignment: Qt.AlignRight
                    text: model.statusMessage
                    font.bold: true
                    renderType: Text.NativeRendering
                    color: "lightgrey"
                }

                StatusPic {
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignRight
                    toxStatus: model.status
                }
            }

            MouseArea {
                anchors.fill:parent
                onClicked: list.currentIndex = index
            }
        }

        onCurrentItemChanged: {
            var f = friendListModel[list.currentIndex]
            console.log("Got f: " + f);
            root.friendSelected(f)
        }
    }
}
