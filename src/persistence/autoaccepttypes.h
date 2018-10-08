/*
    Copyright © 2018 by The qTox Project Contributors

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

#ifndef AUTOACCEPTTYPES_H
#define AUTOACCEPTTYPES_H

#include <QFlag>

enum class AutoAcceptCall
{
    None = 0x00,
    Audio = 0x01,
    Video = 0x02,
    AV = Audio | Video
};
Q_DECLARE_FLAGS(AutoAcceptCallFlags, AutoAcceptCall)
Q_DECLARE_OPERATORS_FOR_FLAGS(AutoAcceptCallFlags)

#endif // AUTOACCEPTTYPES_H
