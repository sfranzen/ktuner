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

import QtQuick 2.3
import QtQuick.Controls 1.4
// import org.kde.ktuner 1.0

// Defines the tuner view that displays information obtained from KTuner
Rectangle {
    id: root
    property color uiColor: "orange"
    SystemPalette { id: palette }
    color: palette.shadow
    height: 300
    width: height * .75
    Item {
        id: noteInfo
        width: parent.width / 2
        height: parent.height / 2
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height / 4
        TunerText {
            id: note
            color: uiColor
            font.pointSize: parent.height / 2
            anchors.left: parent.left
            anchors.baseline: parent.verticalCenter
        }
        TunerText {
            id: octave 
            color: uiColor
            font.pointSize: note.font.pointSize / 2
            anchors.top: note.verticalCenter
            anchors.left: note.right
        }        
    }
    Item {
        id: infoBar
        height: 20
        width: gauge.width
        anchors.bottomMargin: 10
        anchors.bottom: gauge.top
        anchors.horizontalCenter: gauge.horizontalCenter
        TunerText {
            id: frequency
            color: uiColor
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
        }
        TunerText {
            id: deviation
            color: uiColor
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    TunerGauge {
        id: gauge
        barColor: uiColor
//         width: .8 * parent.width
        height: 30
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
    }
    Connections {
        target: tuner
        onNewResult: {
            note.text = result.note.name
            octave.text = result.note.octave
            frequency.text = result.frequency.toFixed(2) + " Hz"
            gauge.value = result.deviation
            deviation.text = result.deviation.toFixed(1) + " c"
            if (Math.abs(result.deviation) < 5) {
                uiColor = "lime"
            } else {
                uiColor = "orange"
            }
        }
    }
}
