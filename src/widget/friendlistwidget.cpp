/*
    Copyright © 2014-2019 by The qTox Project Contributors

    This file is part of qTox, a Qt-based graphical interface for Tox.

    qTox is libre software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    qTox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with qTox.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "friendlistwidget.h"
#include "widget.h"
#include "src/friendlist.h"
#include "src/model/friend.h"
#include "src/model/group.h"
#include "src/model/status.h"
#include "src/persistence/settings.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QGridLayout>
#include <QMimeData>
#include <QQmlContext>
#include <QQuickItem>
#include <QTimer>
#include <cassert>

class ObjectNameRetriever : public QObject
{
    Q_OBJECT

public:
    ObjectNameRetriever(QObject* parent = nullptr)
        : QObject(parent)
    {}
public slots:
    QString getClassName(QObject* obj) const {
        return obj ? obj->metaObject()->className(): "";
    }
};

FriendListWidget::FriendListWidget(QWidget* parent, bool groupsOnTop)
    : QQuickWidget(parent)
{
    updateModelContents();

    auto objectNameRetriever = new ObjectNameRetriever(this);

    auto ctxt = rootContext();
    ctxt->setContextProperty("objectNameRetriever", objectNameRetriever);

    setSource(QUrl::fromLocalFile("../qml/FriendListWidget.qml"));
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setClearColor(Qt::transparent);
    setResizeMode(QQuickWidget::SizeRootObjectToView);


    QObject* root = rootObject();
    connect(root, SIGNAL(friendSelected(QVariant)), this, SLOT(onFriendSelected(QVariant)));
    show();
}

FriendListWidget::~FriendListWidget()
{
}

void FriendListWidget::setMode(SortingMode mode)
{
}

FriendListWidget::SortingMode FriendListWidget::getMode() const
{
    return FriendListWidget::SortingMode::Name;
}

void FriendListWidget::addGroupWidget(Group* g)
{
    if (std::find(groups.begin(), groups.end(), g) != groups.end()) {
        qWarning() << "Adding duplicate friend to friendlist";
        return;
    }

    groups.push_back(g);

    updateModelContents();
}

void FriendListWidget::addFriendWidget(Friend* f, ::Status::Status s, int circleIndex)
{
    if (std::find(friends.begin(), friends.end(), f) != friends.end()) {
        qWarning() << "Adding duplicate friend to friendlist";
        return;
    }

    friends.push_back(f);

    updateModelContents();
}

void FriendListWidget::removeGroupWidget(Group* g)
{
    const auto eraseIt = std::remove(groups.begin(), groups.end(), g);
    groups.erase(eraseIt, groups.end());
}

void FriendListWidget::removeFriendWidget(Friend* f)
{
    const auto eraseIt = std::remove(friends.begin(), friends.end(), f);
    friends.erase(eraseIt, friends.end());
}

void FriendListWidget::addCircleWidget(int id)
{
}

void FriendListWidget::addCircleWidget(FriendWidget* friendWidget)
{
}

void FriendListWidget::removeCircleWidget(CircleWidget* widget)
{
}

void FriendListWidget::searchChatrooms(const QString& searchString, bool hideOnline,
                                       bool hideOffline, bool hideGroups)
{
}

void FriendListWidget::renameGroupWidget(GroupWidget* groupWidget, const QString& newName)
{
}

void FriendListWidget::renameCircleWidget(CircleWidget* circleWidget, const QString& newName)
{
}

void FriendListWidget::onFriendWidgetRenamed(FriendWidget* friendWidget)
{
}

void FriendListWidget::onGroupchatPositionChanged(bool top)
{
}

void FriendListWidget::cycleContacts(GenericChatroomWidget* activeChatroomWidget, bool forward)
{
}

void FriendListWidget::dragEnterEvent(QDragEnterEvent* event)
{
}

void FriendListWidget::dropEvent(QDropEvent* event)
{
}

void FriendListWidget::dayTimeout()
{
}

void FriendListWidget::moveWidget(FriendWidget* widget, ::Status::Status s, bool add)
{
}

void FriendListWidget::updateActivityTime(const QDateTime& time)
{
}

// update widget after add/delete/hide/show
void FriendListWidget::reDraw()
{
}

void FriendListWidget::onFriendSelected(QVariant f_)
{
    Friend* f = f_.value<Friend*>();
    if (f) {
        qDebug() << "Selected friend " << f->getDisplayedName();
        emit friendSelected(f);
    }

    Group* g = f_.value<Group*>();
    if (g) {
        qDebug() << "Selected group " << g->getDisplayedName();
        emit groupSelected(g);
    }

}

void FriendListWidget::setUnselected()
{

}

void FriendListWidget::updateModelContents()
{
    QList<QObject*> contents;

    std::transform(friends.begin(), friends.end(), std::back_inserter(contents),
        [] (Friend* f) {
            return static_cast<QObject*>(f);
        });

    std::transform(groups.begin(), groups.end(), std::back_inserter(contents),
        [] (Group* g) {
            qDebug() << "Adding group " << g;
            return static_cast<QObject*>(g);
        });

    auto ctxt = rootContext();
    ctxt->setContextProperty("friendListModel", QVariant::fromValue(contents));
}

bool FriendListWidget::hasContact(const ContactId& contactId)
{
    const auto friendIt = std::find_if(friends.begin(), friends.end(), [&] (Friend* f) {
        return f->getPersistentId() == contactId;
    });

    if (friendIt != friends.end()) {
        return true;
    }

    const auto groupIt = std::find_if(groups.begin(), groups.end(), [&] (Group* g) {
        return g->getPersistentId() == contactId;
    });

    return groupIt != groups.end();
}

#include "friendlistwidget.moc"
