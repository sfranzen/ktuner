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

import QtQuick 2.1
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles.Plasma 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.ktuner 1.0

Rectangle {
    FontLoader { id: lcd; source: "qrc:/lcd.ttf" }
    height: 400
    width: 600
    SystemPalette { id: palette }
    color: palette.shadow
    TunerView { 
        width: parent.width / 3
        height: parent.height
        anchors.top: parent.top
        anchors.left: parent.left
    }
    SpectrumChart {
        width: 2 * parent.width / 3
        height: parent.height
        anchors.top: parent.top
        anchors.right: parent.right
    }
}
