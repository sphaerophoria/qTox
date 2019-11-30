import QtQuick 2.13
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.13
import org.qtox.status 1.0

Item {
    id: root
    signal friendSelected(variant f)
    signal groupSelected(variant g)
    signal groupQuit(variant g)
    signal groupCreated()
    signal reloadWidget()

    function mergeContacts(friends, groups) {
        var contacts = []

        for (var i = 0; i < groups.length; ++i) {
            contacts.push({
                type: "group",
                contact: groups[i],
            })
        }
        for (var i = 0; i < friends.length; ++i) {
            contacts.push({
                type: "friend",
                contact: friends[i],
            })
        }

        return contacts
    }

    ListView {
        id: list
        z: 1

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds

        model: mergeContacts(friendListModel.friends, friendListModel.groups)

        delegate: Loader {
            width: parent.width
            source: {
                return modelData.type == "friend"
                    ? "ContactListFriend.qml"
                    : "ContactListGroup.qml"
            }
        }

        onCurrentItemChanged: {
            var item = list.model[list.currentIndex]
            if (item.type == "group") {
                root.groupSelected(item.contact)
            } else if (item.type == "friend") {
                root.friendSelected(item.contact)
            }
        }
    }

    MouseArea {
        z: 0
        anchors.fill:parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onClicked: {
            if (mouse.button === Qt.RightButton) {
                contextMenu.popup()
            }
        }

        Menu {
            id: contextMenu

            MenuItem {
                text: qsTr("Create new group...")
                onTriggered: {
                    // FIXME: we should support group rename here
                    root.groupCreated()
                }
            }

            MenuItem { text: qsTr("Add new circle...") }

            MenuItem {
                text: "Reload widget"
                onTriggered: {
                    root.reloadWidget()
                }
            }
        }
    }
}
