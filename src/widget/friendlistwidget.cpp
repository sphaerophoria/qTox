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
#include "src/model/circlemanager.h"
#include "src/persistence/settings.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QGridLayout>
#include <QMimeData>
#include <QQmlContext>
#include <QQuickItem>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QQmlEngine>
#include <cassert>

class FriendListModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QObject*> friends READ getFriends NOTIFY friendsChanged)
    Q_PROPERTY(QList<QObject*> groups READ getGroups NOTIFY groupsChanged)
    Q_PROPERTY(QObject* circleManager MEMBER circleManager);
public:

    FriendListModel(CircleManager* circleManager, QObject* parent = nullptr)
        : QObject(parent)
        , circleManager(circleManager)
    {}

    void addFriend(Friend* f)
    {
        if (std::find(friends.begin(), friends.end(), f) != friends.end()) {
            qWarning() << "Adding duplicate friend to friendlist";
            return;
        }

        friends.push_back(f);

        emit friendsChanged();
    }

    void removeFriend(Friend* f)
    {
        const auto eraseIt = std::remove(friends.begin(), friends.end(), f);
        friends.erase(eraseIt, friends.end());
        emit friendsChanged();
    }

    void addGroup(Group* g)
    {
        if (std::find(groups.begin(), groups.end(), g) != groups.end()) {
            qWarning() << "Adding duplicate friend to friendlist";
            return;
        }

        groups.push_back(g);
        emit groupsChanged();
    }

    void removeGroup(Group* g)
    {
        const auto eraseIt = std::remove(groups.begin(), groups.end(), g);
        groups.erase(eraseIt, groups.end());
        emit groupsChanged();
    }

signals:
    void friendsChanged();
    void groupsChanged();

public slots:
    QList<QObject*> getFriends()
    {
        QList<QObject*> ret;
        std::transform(friends.begin(), friends.end(), std::back_inserter(ret), [] (Friend* f) {
            return static_cast<QObject*>(f);
        });
        return ret;
    }

    QList<QObject*> getGroups()
    {
        QList<QObject*> ret;
        std::transform(groups.begin(), groups.end(), std::back_inserter(ret), [] (Group* g) {
            return static_cast<QObject*>(g);
        });
        return ret;
    }

private:
    std::vector<Friend*> friends;
    std::vector<Group*> groups;
    QObject* circleManager;
};

FriendListWidget::FriendListWidget(QWidget* parent, CircleManager* circleManager, bool groupsOnTop)
    : QQuickWidget(parent)
    , model(new FriendListModel(circleManager, this))
{
    auto ctxt = rootContext();
    ctxt->setContextProperty("friendListModel", model);

    setAttribute(Qt::WA_AlwaysStackOnTop);
    setClearColor(Qt::transparent);
    setResizeMode(QQuickWidget::SizeRootObjectToView);

    loadQml();
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
    model->addGroup(g);
}

void FriendListWidget::addFriendWidget(Friend* f, ::Status::Status s, int circleIndex)
{
    model->addFriend(f);
}

void FriendListWidget::removeGroupWidget(Group* g)
{
    model->removeGroup(g);
}

void FriendListWidget::removeFriendWidget(Friend* f)
{
    model->removeFriend(f);
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
        emit friendSelected(f);
    } else {
        qWarning() << "Invalid type returned from FriendListWidget";
    }
}

void FriendListWidget::onGroupSelected(QVariant g_)
{
    Group* g = g_.value<Group*>();
    if (g) {
        emit groupSelected(g);
    } else {
        qWarning() << "Invalid type returned from FriendListWidget";
    }
}

void FriendListWidget::onGroupQuit(QVariant g_)
{
    qDebug() << "Attempting to quit group";
    Group* g = g_.value<Group*>();
    if (g) {
        emit groupQuit(g);
    } else {
        qWarning() << "Invalid type returned from FriendListWidget";
    }
}

void FriendListWidget::onGroupCreated()
{
    emit groupCreated();
}

void FriendListWidget::onWidgetReload()
{
    QTimer::singleShot(0, this, [=] {
        loadQml();
    });
}

void FriendListWidget::loadQml()
{
    engine()->clearComponentCache();
    setSource(QUrl::fromLocalFile("../qml/FriendListWidget.qml"));

    QObject* root = rootObject();
    connect(root, SIGNAL(friendSelected(QVariant)), this, SLOT(onFriendSelected(QVariant)));
    connect(root, SIGNAL(groupSelected(QVariant)), this, SLOT(onGroupSelected(QVariant)));
    connect(root, SIGNAL(groupQuit(QVariant)), this, SLOT(onGroupQuit(QVariant)));
    connect(root, SIGNAL(groupCreated()), this, SLOT(onGroupCreated()));
    connect(root, SIGNAL(reloadWidget()), this, SLOT(onWidgetReload()));
}

void FriendListWidget::setUnselected()
{

}

bool FriendListWidget::hasContact(const ContactId& contactId)
{
    //FIXME: implement
    //const auto friendIt = std::find_if(friends.begin(), friends.end(), [&] (Friend* f) {
    //    return f->getPersistentId() == contactId;
    //});

    //if (friendIt != friends.end()) {
    //    return true;
    //}

    //const auto groupIt = std::find_if(groups.begin(), groups.end(), [&] (Group* g) {
    //    return g->getPersistentId() == contactId;
    //});

    //return groupIt != groups.end();
    return true;
}

#include "friendlistwidget.moc"
