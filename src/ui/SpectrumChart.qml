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
import QtCharts 2.0

// Enclose the chart in a rectangle of the same background color to eliminate
// the white border shown by default
Rectangle {
    property real xRange: 1000
    property real yRange: 500
    SystemPalette { id: palette }
    color: palette.shadow
    ChartView {
        id: chart
        anchors.fill: parent
        theme: ChartView.ChartThemeDark
        backgroundColor: palette.shadow
        backgroundRoundness: 0
        antialiasing: true
        ValueAxis {
            id: axisY
            titleText: "Power"
            min: 0
            max: min + yRange
        }
        ValueAxis {
            id: axisX
            titleText: "Frequency (Hz)"
            min: 1
            max: min + xRange
        }
        LineSeries {
            id: spectrum
            name: "Power density"
            axisX: axisX
            axisY: axisY
            width: 1
        }
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            property real oldX;
            property real oldY;
            property real oldXMin;
            property real oldYMin;
            onPressed: {
                oldXMin = axisX.min;
                oldYMin = axisY.min;
                oldX = mouse.x;
                oldY = mouse.y;
            }
            onPositionChanged: {
                if (pressed) {
                    axisX.min = oldXMin + (oldX - mouseX) * xRange / chart.plotArea.width;
                    axisX.max = axisX.min + xRange;
                    axisY.min = oldYMin - (oldY - mouseY) * yRange / chart.plotArea.height;
                    axisY.max = axisY.min + yRange;
                }
            }
        }
        Connections {
            target: tuner
            onNewResult: {
                tuner.updateSpectrum(chart.series(0));
            }
        }
    }
}
