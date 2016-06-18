import QtQuick 2.5
import QtCharts 2.0

ChartView {
    SystemPalette { id: palette }
    theme: ChartView.ChartThemeDark
    backgroundColor: palette.shadow
    backgroundRoundness: 0
    antialiasing: true
    ValueAxis {
        id: axisX
        titleText: "Frequency (Hz)"
        min: 0
        max: 1000
    }
    ValueAxis {
        id: axisY
        titleText: "Power"
        min: 0
        max: 1
    }
    LineSeries {
        id: spectrum
        name: "Power density spectrum"
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
