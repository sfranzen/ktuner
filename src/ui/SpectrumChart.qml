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
    property real yRange: 2
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
            titleText: i18n("Power (-)")
            min: 0
            max: min + yRange
        }
        ValueAxis {
            id: axisX
            titleText: i18n("Frequency (Hz)")
            min: 1
            max: min + xRange
        }
        LineSeries {
            id: spectrum
            name: i18n("Power spectrum")
            property real maxFreq: 0
            axisX: axisX
            axisY: axisY
            width: 1
            color: "lime"
            useOpenGL: true
            onPointsReplaced: {
                var newFreq = at(count - 1).x;
                if (maxFreq != newFreq)
                    maxFreq = newFreq;
            }
        }
        ScatterSeries {
            id: harmonics
            name: i18n("Harmonics")
            axisX: axisX
            axisY: axisY
            markerSize: 8
            borderWidth: 0
            color: "white"
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
                    var newX = Math.max(1, oldXMin + (oldX - mouseX) * xRange / chart.plotArea.width);
                    var newY = Math.max(0, oldYMin - (oldY - mouseY) * yRange / chart.plotArea.height);
                    axisX.min = newX;
                    axisX.max = axisX.min + xRange;
                }
            }
            onWheel: {
                var newRange = xRange;
                if (wheel.angleDelta.y > 0) {
                    newRange *= 0.75;
                } else {
                    newRange /= 0.75;
                }
                newRange = Math.min(newRange, spectrum.maxFreq);
                xRange = newRange;
            }
        }
        Connections {
            target: tuner
            onNewResult: {
                for (var i = 0; i < chart.count; ++i) {
                    tuner.updateSpectrum(chart.series(i));
                }
            }
        }
    }
}
