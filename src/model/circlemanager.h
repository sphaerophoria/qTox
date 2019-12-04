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

#include <limits>
#include <unordered_map>

class IFriendSettings;
class ICircleSettings;
class Friend;

using CircleId = NamedType<int, struct CircleIdTag, Orderable, Hashable>;

class CircleManager
{
public:
    static constexpr CircleId none = CircleId(std::numeric_limits<int>::min());
    CircleManager(IFriendSettings& friendSettings, ICircleSettings& circleSettings);

    std::vector<CircleId> getCircles();
    CircleId addCircle();
    void removeCircle(CircleId id);
    QString getCircleName(CircleId id);
    void setCircleName(CircleId id, QString name);
    void addFriendToCircle(Friend const* f, CircleId circle);
    void removeFriendFromCircle(Friend const* f, CircleId circle);
    CircleId getFriendCircle(Friend const* f);
    bool getCircleExpanded(CircleId id) const;
    void setCircleExpanded(CircleId id, bool expanded);
private:
    IFriendSettings& friendSettings;
    ICircleSettings& circleSettings;
    struct CircleData
    {
        QString name;
        bool circleExpanded = false;
    };

    size_t nextId = 0;
    std::unordered_map<CircleId, CircleData> circleData;
    std::unordered_map<Friend const*, CircleId> friendCircle;
};

#endif /*CIRCLE_MANAGER_H*/
