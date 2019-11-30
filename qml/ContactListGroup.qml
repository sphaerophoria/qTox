import QtQuick 2.13
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.13
import org.qtox.status 1.0
import QtQuick.Controls.Styles 1.4

Item {
    width: parent.width
    height: 20

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            state: "normal"
            id: groupName

            Text {
                id: groupNameLabel
                text: modelData.contact.name
                renderType: Text.NativeRendering
                font.bold: true
                color: "white"
            }

            TextField {
                id: groupNameEditor
                visible: false
                text: modelData.contact.name
                style: TextFieldStyle {
                    background: Item{}
                }
                onAccepted: {
                    modelData.contact.name = text
                    groupName.state = "normal"
                }
            }

            states: [
                State {
                    name: "editing"
                    PropertyChanges {
                        target: groupNameLabel
                        visible: false
                    }
                    PropertyChanges {
                        target: groupNameEditor
                        visible: true
                    }
                },
                State {
                    name: "normal"
                    PropertyChanges {
                        target: groupNameLabel
                        visible: true
                    }
                    PropertyChanges {
                        target: groupNameEditor
                        visible: false
                    }
                }
            ]
        }

        Text {
            Layout.alignment: Qt.AlignRight
            text: "" + modelData.contact.peersCount + " user(s) in chat"
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

            MenuItem {
                text: qsTr("Set title...")
                onTriggered: {
                    groupName.state = "editing"
                }
            }

            MenuItem {
                text: qsTr("Quit group")
                onTriggered: {
                    root.groupQuit(modelData.contact)
                }
            }
        }
    }
}
