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
    color: chart.backgroundColor
    BaseChart {
        id: chart
        ValueAxis {
            id: axisY
            titleText: i18n("SNAC (-)")
            min: -1
            max: 1
        }
        ValueAxis {
            id: axisX
            titleText: i18n("Delay (number of sampled periods)")
            min: 0
            max: min + chart.xRange
        }
        SpectrumSeries {
            id: snac
            name: i18n("Normalized autocorrelation")
            axisX: axisX
            axisY: axisY
        }
        MouseArea {
            anchors.fill: parent
            onWheel: chart.xZoom(wheel, snac.xMax())
        }
        Connections {
            target: tuner
            onNewResult: {
                tuner.updateAutocorrelation(chart.series(0));
            }
        }
    }
}
