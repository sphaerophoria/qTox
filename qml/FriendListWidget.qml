import QtQuick 2.13
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.13
import org.qtox.status 1.0

Item {
    id: root
    signal friendSelected(variant f)

    function mergeContacts(friends, groups) {
        var contacts = []

        for (var i = 0; i < groups.length; ++i) {
            contacts.push({
                type: "group",
                contact: groups[i]
            })
        }
        for (var i = 0; i < friends.length; ++i) {
            contacts.push({
                type: "friend",
                contact: friends[i]
            })
        }

        return contacts
    }

    ListView {
        id: list

        anchors.fill: parent
        boundsBehavior: Flickable.StopAtBounds

        model: mergeContacts(friendListModel.friends, friendListModel.groups)

        onModelChanged: {
            var numGroups = 0;
            var numFriends = 0;
            for (var i = 0; i < model.length; ++i) {
                console.log("Item: " + model[i])
            }
        }

        Component.onCompleted: {
            console.log("Got model " + model)
        }

        // FIXME: loader based on type of item somehow
        delegate: Loader {
            width: parent.width
            source: {
                console.log("modelData: " + modelData.type);
                return modelData.type == "friend"
                    ? "ContactListFriend.qml"
                    : "ContactListGroup.qml"
            }
        }

        onCurrentItemChanged: {
            var f = list.model[list.currentIndex].contact
            console.log("Got f: " + f);
            root.friendSelected(f)
        }
    }
}
