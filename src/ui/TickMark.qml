/*
 * Copyright 2016 Steven Franzen <sfranzen85@gmail.com>
 *
 * This file is part of KTuner.
 *
 * KTuner is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * KTuner is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * KTuner. If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.5

Item {
    property color color: "gray"
    property bool minor: false
    width: 1
    height: 20
    Canvas {
        width: parent.width
        height: parent.height
        onPaint: {
            var c = getContext("2d");
            c.fillStyle = color;
            if (minor)
                c.fillRect(0, height/4, width, height/2);
            else
                c.fillRect(0, 0, width, height);
        }
    }    
}
