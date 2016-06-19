import QtQuick 2.5
import QtCharts 2.0

ChartView {
    SystemPalette { id: palette }
    theme: ChartView.ChartThemeDark
    backgroundColor: palette.shadow
    backgroundRoundness: 0
    antialiasing: true
    ValueAxis {
        id: axisY
        titleText: "Power"
        min: 0
        max: 500
    }
    ValueAxis {
        id: axisX
        titleText: "Frequency (Hz)"
        min: 1
        max: 1000
    }
    LineSeries {
        id: spectrum
        name: "Power density"
        axisX: axisX
        axisY: axisY
        width: 1
    }
    Connections {
        target: tuner
        onNewResult: {
            tuner.updateSpectrum(series(0));
        }
    }
}
