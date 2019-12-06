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

#include "src/model/friend.h"

#include "src/persistence/icirclesettings.h"
#include "src/persistence/ifriendsettings.h"

#include <QDebug>
#include <iostream>

namespace
{


} // namespace

CircleManager::CircleManager(
    const std::vector<const Friend*>& initialFriends,
    IFriendSettings& friendSettings,
    ICircleSettings& circleSettings)
    : friendSettings(friendSettings)
    , circleSettings(circleSettings)
{
    loadCircles();
    loadFriendCircles(initialFriends);
}

std::vector<CircleId> CircleManager::getCircles()
{
    return settingsIdToCircleId;
}

CircleId CircleManager::addCircle()
{
    auto circleId = CircleId(nextId++);

    auto settingsId = circleSettings.addCircle();
    assert(settingsId == settingsIdToCircleId.size());
    settingsIdToCircleId.push_back(circleId);
    circleIdToSettingsId[circleId] = settingsId;

    return circleId;
}

void CircleManager::removeCircle(CircleId id)
{
    std::vector<const Friend*> friendsInCircle;
    for (auto const& friendCirclePair : friendCircle)
    {
        if (friendCirclePair.second == id) {
            friendsInCircle.push_back(friendCirclePair.first);
        }
    }

    for (auto const& f : friendsInCircle)
    {
        removeFriendFromCircle(f, id);
    }

    auto settingsId = circleIdToSettingsId[id];
    auto movedSettingsId = circleSettings.removeCircle(settingsId);

    circleIdToSettingsId.erase(id);

    assert(movedSettingsId == settingsIdToCircleId.size() - 1);
    circleIdToSettingsId[settingsIdToCircleId[movedSettingsId]] = settingsId;
    settingsIdToCircleId[settingsId] = settingsIdToCircleId[movedSettingsId];
    settingsIdToCircleId.pop_back();

    emit circleRemoved(id);
}


QString CircleManager::getCircleName(CircleId id)
{
    auto it = circleIdToSettingsId.find(id);
    if (it == circleIdToSettingsId.end())
        return QString();

    return circleSettings.getCircleName(it->second);
}

void CircleManager::setCircleName(CircleId id, QString name)
{
    auto it = circleIdToSettingsId.find(id);
    if (it == circleIdToSettingsId.end())
        return;

    circleSettings.setCircleName(it->second, name);
    emit circleNameChanged(id);
}

void CircleManager::addFriendToCircle(Friend const* f, CircleId circle)
{
    auto it = circleIdToSettingsId.find(circle);
    if (it == circleIdToSettingsId.end())
        return;

    friendCircle[f] = circle;
    friendSettings.setFriendCircleID(f->getPublicKey(), circleIdToSettingsId[circle]);

    emit friendCircleChanged(f);
}

void CircleManager::removeFriendFromCircle(Friend const* f, CircleId circle)
{
    friendCircle.erase(f);
    friendSettings.setFriendCircleID(f->getPublicKey(), -1);

    emit friendCircleChanged(f);
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
    auto it = circleIdToSettingsId.find(id);
    if (it == circleIdToSettingsId.end())
        return false;

    return circleSettings.getCircleExpanded(it->second);
}

void CircleManager::setCircleExpanded(CircleId id, bool expanded)
{
    auto it = circleIdToSettingsId.find(id);
    if (it == circleIdToSettingsId.end())
        return;

    circleSettings.setCircleExpanded(it->second, expanded);

    emit circleExpandedChanged(id);
}

void CircleManager::loadCircles()
{
    auto circleCount = circleSettings.getCircleCount();
    for (int i = 0; i < circleCount; ++i)
    {
        auto circleId = CircleId(nextId++);
        settingsIdToCircleId.push_back(circleId);
        circleIdToSettingsId[circleId] = i;
    }
}

void CircleManager::loadFriendCircles(const std::vector<const Friend*>& initialFriends)
{
    for (auto const& f : initialFriends)
    {
        auto id = friendSettings.getFriendCircleID(f->getPublicKey());
        if (id == -1)
            continue;

        if (id >= settingsIdToCircleId.size()) {
            qWarning() << "Unknown circle id";
            continue;
        }
        auto circleId = settingsIdToCircleId[id];

        friendCircle[f] = circleId;
    }
}

constexpr CircleId CircleManager::none;
