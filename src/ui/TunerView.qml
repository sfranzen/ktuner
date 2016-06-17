import QtQuick 2.3
import QtQuick.Controls 1.4
import org.kde.ktuner 1.0

// Defines the tuner view that displays information obtained from KTuner
Rectangle {
    property color uiColor: "orange"
    SystemPalette { id: palette }
    anchors.fill: parent
    color: palette.shadow
    TunerText {
        id: note
        color: uiColor
        anchors.centerIn: parent
        font.pointSize: parent.height / 2
    }
    TunerText {
        id: octave 
        color: uiColor
        font.pointSize: note.font.pointSize / 2
        anchors.top: note.verticalCenter
        anchors.left: note.right
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
