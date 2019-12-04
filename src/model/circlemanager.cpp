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

#include "circlemanager.h"
#include "src/persistence/icirclesettings.h"
#include "src/persistence/ifriendsettings.h"

#include <iostream>

CircleManager::CircleManager(IFriendSettings& friendSettings, ICircleSettings& circleSettings)
    : friendSettings(friendSettings)
    , circleSettings(circleSettings)
{
}

std::vector<CircleId> CircleManager::getCircles()
{
    std::vector<CircleId> circles;
    circles.reserve(circleData.size());

    std::transform(
        circleData.begin(), circleData.end(),
        std::back_inserter(circles),
        [] (std::pair<CircleId, CircleData> const& item) {
            return item.first;
        });

    return circles;
}

CircleId CircleManager::addCircle()
{
    auto circleId = CircleId(nextId++);
    // FIXME: hook into settings
    circleData[circleId] = CircleData();
    return circleId;
}

void CircleManager::removeCircle(CircleId id)
{
    circleData.erase(id);
}

QString CircleManager::getCircleName(CircleId id)
{
    auto it = circleData.find(id);
    if (it == circleData.end())
        return QString();

    return it->second.name;
}

void CircleManager::setCircleName(CircleId id, QString name)
{
    auto it = circleData.find(id);
    if (it == circleData.end())
        return;

    it->second.name = name;
}

void CircleManager::addFriendToCircle(Friend const* f, CircleId circle)
{
    auto it = circleData.find(circle);
    if (it == circleData.end())
        return;

    friendCircle[f] = circle;
}

void CircleManager::removeFriendFromCircle(Friend const* f, CircleId circle)
{
    friendCircle.erase(f);
}

CircleId CircleManager::getFriendCircle(Friend const* f)
{
    auto it = friendCircle.find(f);
    if (it == friendCircle.end())
        return none;

    return it->second;
}

bool CircleManager::getCircleExpanded(CircleId id) const
{
    auto it = circleData.find(id);
    if (it == circleData.end())
        return false;

    return it->second.circleExpanded;
}

void CircleManager::setCircleExpanded(CircleId id, bool expanded)
{
    auto it = circleData.find(id);
    if (it == circleData.end())
        return;

    it->second.circleExpanded = expanded;
}

constexpr CircleId CircleManager::none;
