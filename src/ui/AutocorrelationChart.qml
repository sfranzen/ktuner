/*
 * Copyright 2018 Steven Franzen <sfranzen85@gmail.com>
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
import QtCharts 2.2

// Enclose the chart in a rectangle of the same background color to eliminate
// the white border shown by default
Rectangle {
    property real xRange: 1000
    SystemPalette { id: palette }
    color: palette.shadow
    ChartView {
        id: chart
        anchors.fill: parent
        theme: ChartView.ChartThemeDark
        backgroundColor: palette.shadow
        backgroundRoundness: 0
        antialiasing: false
        ValueAxis {
            id: axisY
            titleText: i18n("Power (-)")
            min: -1
            max: 1
        }
        ValueAxis {
            id: axisX
            titleText: i18n("Period (s)")
            min: 0
            max: min + xRange
        }
        LineSeries {
            id: spectrum
            name: i18n("Normalized autocorrelation")
            axisX: axisX
            axisY: axisY
            width: 1
            color: "lime"
            function maxFreq() {
                return at(count - 1).x;
            }
        }
        ScatterSeries {
            id: harmonics
            name: i18n("Peaks")
            axisX: axisX
            axisY: axisY
            markerSize: 8
            borderWidth: 0
            color: "white"
        }
        MouseArea {
            anchors.fill: parent
            onWheel: {
                var nextRange = xRange * Math.pow(1.5, -wheel.angleDelta.y / 120);
                xRange = Math.min(spectrum.maxFreq(), nextRange);
            }
        }
        Connections {
            target: tuner
            onNewResult: {
                for (var i = 0; i < chart.count; ++i) {
                    tuner.updateAutocorrelation(chart.series(i));
                }
            }
        }
    }
}
