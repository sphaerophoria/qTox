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

#ifndef ICIRCLE_SETTINGS_H
#define ICIRCLE_SETTINGS_H

#include <QString>

class ICircleSettings
{
public:
    virtual ~ICircleSettings(){}

    virtual int getCircleCount() const = 0;
    virtual int addCircle(const QString& name = QString()) = 0;

    /**
     * @brief Removes cirlce at id
     * @param[in] id
     * @return New size of circle list
     * @note Implementations currently guarantee that this is implemented
     * via a swap and pop. This means that the returned value effectively
     * tells the caller which id replaced the removed id
     */
    virtual int removeCircle(int id) = 0;
    virtual QString getCircleName(int id) const = 0;
    virtual void setCircleName(int id, const QString& name) = 0;
    virtual bool getCircleExpanded(int id) const = 0;
    virtual void setCircleExpanded(int id, bool expanded) = 0;
};

#endif /*ICIRCLE_SETTINGS_H*/
