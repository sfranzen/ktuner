import QtQuick 2.5
import QtQuick.Layouts 1.3

// A gauge that displays a value on a zero-centered axis with tick marks
Item {
    property real value: 0
    property real limit: 50.0
    property color barColor: "lime"
    property color axisColor: "gray"
    property int numTicks: 5
    property int tickHeight: height - 10
    width: 150
    height: 40
    Canvas {
        id: axis
        height: parent.height
        width: parent.width
        anchors.top: parent.top
        property var length: width - 10
        property var margin: 5
        onPaint: {
            var c = getContext("2d");
            c.fillStyle = axisColor;
            c.fillRect(margin, tickHeight/2, length, 1);
            // Tick marks:
            for (var i = 0; i < numTicks - 1; i++) {
                c.fillRect(margin + i * length / (numTicks - 1), 0, 1, tickHeight);
            }
            c.fillRect(margin + length - 1, 0, 1, tickHeight);
            c.fillText(-50, margin - ("-50").length, tickHeight+10);
        }
    }
    Rectangle {
        id: bar
        height: tickHeight / 2
        color: barColor
        y: (tickHeight - height) / 2 + 1
    }
    onValueChanged: {
        bar.width = Math.abs(value) * axis.length / (2 * limit)
        if (value >= 0) {
            bar.x = width / 2 + 1
        } else {
            bar.x = width / 2 - bar.width
        }
    }
}
