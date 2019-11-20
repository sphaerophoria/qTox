import QtQuick 2.13
import org.qtox.status 1.0
import StatusPic

Rectangle {
    property int toxStatus: Status.Offline

    Image {
        id: avatar
    }

    Label {
        id: name
    }

    Label {
        id: status
    }

    StatusPic {
        id: pic
    }
}
