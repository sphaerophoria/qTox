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

        onModelChanged: {
            var numGroups = 0;
            var numFriends = 0;
            for (var i = 0; i < model.length; ++i) {
                console.log("Item name " + objectNameRetriever.getClassName(model[i]));
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
                return objectNameRetriever.getClassName(modelData) == "Friend"
                    ? "ContactListFriend.qml"
                    : "ContactListGroup.qml"
            }
        }

        onCurrentItemChanged: {
            var f = friendListModel[list.currentIndex]
            console.log("Got f: " + f);
            root.friendSelected(f)
        }
    }
}
