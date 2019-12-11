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

std::vector<Circle*> CircleManager::getCircles()
{
    std::vector<Circle*> ret;
    ret.reserve(circles.size());

    std::transform(circles.begin(), circles.end(), std::back_inserter(ret), [] (std::unique_ptr<Circle>& circle) {
        return circle.get();
    });

    return ret;
}

Circle* CircleManager::addCircle()
{
    auto settingsId = circleSettings.addCircle();
    auto circle = std::unique_ptr<Circle>(new Circle(circleSettings, settingsId, {}));
    auto ret = circle.get();
    circles.push_back(std::move(circle));

    emit circlesChanged();

    return ret;
}

void CircleManager::removeCircle(Circle* circle)
{
    std::vector<const Friend*> friendsInCircle;
    for (auto const& friendCirclePair : friendCircle)
    {
        if (friendCirclePair.second == circle) {
            friendsInCircle.push_back(friendCirclePair.first);
        }
    }

    for (auto const& f : friendsInCircle)
    {
        removeFriendFromCircle(f, circle);
    }

    auto settingsId = circle->getSettingsId({});
    auto movedSettingsId = circleSettings.removeCircle(settingsId);

    auto removedIt = std::remove_if(circles.begin(), circles.end(), [&](std::unique_ptr<Circle>& item) {
        return item.get() == circle;
    });
    circles.erase(removedIt, circles.end());

    auto movedCircle = std::find_if(circles.begin(), circles.end(), [&] (std::unique_ptr<Circle>& circle) {
        return circle->getSettingsId({}) == movedSettingsId;
    });

    if (movedCircle != circles.end()) {
        (*movedCircle)->setSettingsId(settingsId, {});
    } else {
        assert(movedSettingsId == settingsId);
    }

    emit circlesChanged();
}

void CircleManager::addFriendToCircle(Friend const* f, Circle* circle)
{
    friendCircle[f] = circle;
    friendSettings.setFriendCircleID(f->getPublicKey(), circle->getSettingsId({}));

    emit friendCircleChanged(f);
}

void CircleManager::removeFriendFromCircle(Friend const* f, Circle* circle)
{
    auto friendCircleIt = friendCircle.find(f);
    if (friendCircleIt == friendCircle.end() || friendCircleIt->second != circle) {
        qWarning() << "Friend not in circle";
        return;
    }

    friendCircle.erase(f);
    friendSettings.setFriendCircleID(f->getPublicKey(), -1);

    emit friendCircleChanged(f);
}

Circle* CircleManager::getFriendCircle(Friend const* f)
{
    auto it = friendCircle.find(f);
    if (it == friendCircle.end())
        return nullptr;

    return it->second;
}

void CircleManager::loadCircles()
{
    auto circleCount = circleSettings.getCircleCount();
    for (int i = 0; i < circleCount; ++i)
    {
        circles.push_back(std::unique_ptr<Circle>(new Circle(circleSettings, i, {})));
    }
}

void CircleManager::loadFriendCircles(const std::vector<const Friend*>& initialFriends)
{
    for (auto const& f : initialFriends)
    {
        auto id = friendSettings.getFriendCircleID(f->getPublicKey());
        if (id == -1)
            continue;

        auto circleIt = std::find_if(circles.begin(), circles.end(), [&] (std::unique_ptr<Circle>& circle) {
            return circle->getSettingsId({}) == id;
        });

        if (circleIt == circles.end()) {
            qWarning() << "Unknown circle id";
            continue;
        }

        friendCircle[f] = circleIt->get();
    }
}

int Circle::getSettingsId(CircleManagerKey)
{
    return settingsId;
}

void Circle::setSettingsId(int settingsId, CircleManagerKey)
{
    this->settingsId = settingsId;
}

QString Circle::getName()
{
    return circleSettings.getCircleName(settingsId);
}

void Circle::setName(QString name)
{
    circleSettings.setCircleName(settingsId, name);
    emit nameChanged();
}

bool Circle::getExpanded() const
{
    return circleSettings.getCircleExpanded(settingsId);
}

void Circle::setExpanded(bool expanded)
{
    circleSettings.setCircleExpanded(settingsId, expanded);
    emit expandedChanged();
}
