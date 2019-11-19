import QtQuick 2.13
import org.qtox.status 1.0

Rectangle {
    property int toxStatus: Status.Offline
    property bool newMessage: false

    function statusString(toxStatus) {
        switch (toxStatus) {
        case Status.Online:
            return "online"
        case Status.Away:
            return "away"
        case Status.Busy:
            return "busy"
        case Status.Blocked:
            return "blocked"
        case Status.Offline:
            return "offline"
        }
        return ""
    }

    function notificationString(newMessage) {
        return newMessage ? "_notification" : ""
    }

    color: "transparent"

    Image {
        source: "qrc:///img/status/"
            + statusString(toxStatus)
            + notificationString(newMessage)
            + ".svg"

        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter

        fillMode: Image.PreserveAspectFit
    }
}
