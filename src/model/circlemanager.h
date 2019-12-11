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

#ifndef CIRCLE_MANAGER_H
#define CIRCLE_MANAGER_H

#include "src/util/strongtype.h"

#include <QObject>

#include <memory>
#include <limits>
#include <unordered_map>

class IFriendSettings;
class ICircleSettings;
class Friend;
class Circle;

class CircleManager : public QObject
{
    Q_OBJECT
public:
    CircleManager(
        const std::vector<Friend const*>& initialFriends,
        IFriendSettings& friendSettings,
        ICircleSettings& circleSettings);

public slots:
    std::vector<Circle*> getCircles();
    Circle* addCircle();
    void removeCircle(Circle* circle);
    void addFriendToCircle(Friend const* f, Circle* circle);
    void removeFriendFromCircle(Friend const* f, Circle* circle);
    Circle* getFriendCircle(Friend const* f);

signals:
    void circlesChanged();
    void friendCircleChanged(const Friend* f);

private:
    void loadCircles();
    void loadFriendCircles(const std::vector<const Friend*>& initialFriends);

    IFriendSettings& friendSettings;
    ICircleSettings& circleSettings;
    size_t nextId = 0;
    std::unordered_map<Friend const*, Circle*> friendCircle;
    std::vector<std::unique_ptr<Circle>> circles;
};

class Circle : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ getName WRITE setName NOTIFY nameChanged);
    Q_PROPERTY(bool expanded READ getExpanded WRITE setExpanded NOTIFY expandedChanged);


public:
    struct CircleManagerKey {
        friend class CircleManager;
        private:
            CircleManagerKey() {}
            CircleManagerKey(CircleManagerKey const&) = default;
    };
    Circle(ICircleSettings& circleSettings, int settingsId, CircleManagerKey)
        : circleSettings(circleSettings)
        , settingsId(settingsId)
    {}

    Circle(Circle const& other) = delete;
    Circle& operator=(Circle const& other) = delete;
    Circle(Circle&& other) = delete;
    Circle& operator=(Circle&& other) = delete;

    int getSettingsId(CircleManagerKey);
    void setSettingsId(int id, CircleManagerKey);

    QString getName();
    void setName(QString name);
    bool getExpanded() const;
    void setExpanded(bool expanded);

signals:
    void nameChanged();
    void expandedChanged();

private:

    ICircleSettings& circleSettings;
    int settingsId;
};

#endif /*CIRCLE_MANAGER_H*/
