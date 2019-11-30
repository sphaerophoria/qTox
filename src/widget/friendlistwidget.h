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

#ifndef FRIENDLISTWIDGET_H
#define FRIENDLISTWIDGET_H

#include "genericchatitemlayout.h"
#include "src/core/core.h"
#include "src/model/status.h"
#include "src/persistence/settings.h"
#include <QWidget>
#include <QQuickWidget>

class QVBoxLayout;
class QGridLayout;
class QPixmap;
class Widget;
class FriendWidget;
class GroupWidget;
class CircleWidget;
class FriendListLayout;
class GenericChatroomWidget;
class CategoryWidget;
class Friend;
class Group;
class FriendListModel;

class FriendListWidget : public QQuickWidget
{
    Q_OBJECT
public:
    using SortingMode = Settings::FriendListSortingMode;
    explicit FriendListWidget(QWidget* parent, bool groupsOnTop = true);
    ~FriendListWidget() override;
    void setMode(SortingMode mode);
    SortingMode getMode() const;

    void addGroupWidget(Group* widget);
    void addFriendWidget(Friend* f, ::Status::Status s, int circleIndex);
    void removeGroupWidget(Group* w);
    void removeFriendWidget(Friend* w);
    void addCircleWidget(int id);
    void addCircleWidget(FriendWidget* widget = nullptr);
    void removeCircleWidget(CircleWidget* widget);
    void searchChatrooms(const QString& searchString, bool hideOnline = false,
                         bool hideOffline = false, bool hideGroups = false);

    void cycleContacts(GenericChatroomWidget* activeChatroomWidget, bool forward);

    void updateActivityTime(const QDateTime& date);
    void reDraw();
    void setUnselected();

    bool hasContact(const ContactId& id);

signals:
    void onCompactChanged(bool compact);
    void connectCircleWidget(CircleWidget& circleWidget);
    void searchCircle(CircleWidget& circleWidget);
    void friendSelected(Friend* f);
    void groupSelected(Group* g);
    void groupQuit(Group* g);
    void groupCreated();

public slots:
    void renameGroupWidget(GroupWidget* groupWidget, const QString& newName);
    void renameCircleWidget(CircleWidget* circleWidget, const QString& newName);
    void onFriendWidgetRenamed(FriendWidget* friendWidget);
    void onGroupchatPositionChanged(bool top);
    void moveWidget(FriendWidget* w, ::Status::Status s, bool add = false);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void dayTimeout();
    void onFriendSelected(QVariant f);
    void onGroupSelected(QVariant g);
    void onGroupQuit(QVariant g);
    void onGroupCreated();
    void onWidgetReload();

private:
    void loadQml();
    FriendListModel* model;
};

#endif // FRIENDLISTWIDGET_H
