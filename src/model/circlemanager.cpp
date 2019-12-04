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

#pragma GCC diagnostic ignored "-Wreturn-type"

CircleManager::CircleManager(IFriendSettings& friendSettings, ICircleSettings& circleSettings)
    : friendSettings(friendSettings)
    , circleSettings(circleSettings)
{
}

std::vector<CircleId> CircleManager::getCircles()
{

}

template <typename T>
struct Test
{
};

template <typename T, typename Tag, template <typename, typename> class... Properties>
struct Test<NamedType<T, Tag, Properties...>>
{
    using NT = NamedType<T, Tag, Properties...>;

    template <typename U, typename V>
    typename std::enable_if<std::is_base_of<NT, Hashable<U, V>>::value, size_t>::type
    testFn()
    {
        return 1;
    }

    static const bool value = true;
};


CircleId CircleManager::addCircle()
{
    auto hash = std::hash<CircleId>()(CircleId(0));
}

void CircleManager::removeCircle(CircleId id)
{

}

QString CircleManager::getCircleName(CircleId id)
{

}

void CircleManager::setCircleName(CircleId id, QString name)
{

}

void CircleManager::addFriendToCircle(Friend const* f, CircleId circle)
{

}

void CircleManager::removeFriendFromCircle(Friend const* f, CircleId circle)
{

}

CircleId CircleManager::getFriendCircle(Friend const* f)
{

}

constexpr CircleId CircleManager::none;
